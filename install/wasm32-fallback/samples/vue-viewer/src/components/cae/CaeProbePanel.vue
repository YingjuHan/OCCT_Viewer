<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, PropType } from 'vue';
import type { IMeshPropertyValuesInfo, ISelectionMeshEntity, MeshAnimationChangedEvent, ObjectId } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import { InteractiveCaeMesh, MeshEntityType, UserEvents } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import type { MeshSummary, ViewerService } from '../../services/viewer.service';

type ProbeRecord = {
  id: string;
  meshId: ObjectId;
  entity: ISelectionMeshEntity;
  createdAt: number;
  updatedAt: number;
};

type ProbePropertyEntry = {
  propId: string;
  name: string;
  isActive: boolean;
  info: IMeshPropertyValuesInfo;
};

const props = defineProps({
  viewerService: {
    type: Object as PropType<ViewerService>,
    required: true
  }
});

const meshes = ref<MeshSummary[]>([]);
const selectedMeshId = ref<ObjectId | ''>('');
const probes = ref<ProbeRecord[]>([]);
const activeProbeId = ref('');
const jsonText = ref('');
const notice = ref<{ type: 'info' | 'warning' | 'error' | 'success'; text: string } | null>(null);

const createId = () => {
  if (typeof crypto !== 'undefined' && 'randomUUID' in crypto) {
    return (crypto as Crypto).randomUUID();
  }
  return `probe-${Date.now().toString(16)}-${Math.random().toString(16).slice(2)}`;
};

const setNotice = (text: string, type: 'info' | 'warning' | 'error' | 'success' = 'info') => {
  notice.value = { text, type };
  setTimeout(() => {
    if (notice.value?.text === text) {
      notice.value = null;
    }
  }, 3000);
};

const refreshMeshes = () => {
  meshes.value = props.viewerService.listMeshSummaries();
  if (meshes.value.length === 0) {
    selectedMeshId.value = '';
    return;
  }
  if (!selectedMeshId.value || !meshes.value.find((mesh) => mesh.id === selectedMeshId.value)) {
    selectedMeshId.value = meshes.value[0].id;
  }
};

const getMeshById = (id: ObjectId): InteractiveCaeMesh | null => {
  const obj = props.viewerService.getInteractiveObject(id);
  return obj instanceof InteractiveCaeMesh ? obj : null;
};

const activeProbe = computed(() => probes.value.find((probe) => probe.id === activeProbeId.value) ?? null);
const hasMesh = computed(() => meshes.value.length > 0);
const hasProps = computed(() => (activeProbe.value?.entity?.propsInfo?.size ?? 0) > 0);

const formatValue = (value: number): string => {
  if (!Number.isFinite(value) || Math.abs(value) > 1e100) {
    return 'n/a';
  }
  const abs = Math.abs(value);
  if (abs >= 1e4 || (abs > 0 && abs < 1e-3)) {
    return value.toExponential(4);
  }
  return value.toFixed(6);
};

const formatPos = (pos: { x: number; y: number; z: number }): string => {
  const unit = props.viewerService.unitManager?.length;
  if (unit) {
    return `${unit.format(pos.x)}, ${unit.format(pos.y)}, ${unit.format(pos.z)}`;
  }
  return `${pos.x}, ${pos.y}, ${pos.z}`;
};

const getEntityTypeLabel = (type: number): string => {
  const mapped = (MeshEntityType as Record<number, string>)[type];
  return mapped ?? `Type ${type}`;
};

const getActivePropIds = (entity: ISelectionMeshEntity): Set<string> => {
  const ids = new Set<string>();
  if (!entity.prsPropsInfo) {
    return ids;
  }
  for (const info of entity.prsPropsInfo.values()) {
    if (info?.propertyId) {
      ids.add(info.propertyId);
    }
  }
  return ids;
};

const propertyEntries = computed<ProbePropertyEntry[]>(() => {
  const probe = activeProbe.value;
  if (!probe) {
    return [];
  }
  const activeIds = getActivePropIds(probe.entity);
  const entries: ProbePropertyEntry[] = [];
  probe.entity.propsInfo.forEach((info, propId) => {
    const name = props.viewerService.getMeshPropertyName(probe.meshId, propId) || propId;
    entries.push({
      propId,
      name,
      isActive: activeIds.has(propId),
      info
    });
  });
  return entries;
});

const pickProbe = async () => {
  notice.value = null;
  if (!selectedMeshId.value) {
    setNotice('Select a mesh first', 'warning');
    return;
  }
  const mesh = getMeshById(selectedMeshId.value);
  if (!mesh) {
    setNotice('Mesh is not available', 'warning');
    return;
  }
  const entity = mesh.getDetectedEntity();
  if (!entity || entity.entityId === -1) {
    setNotice('Hover a mesh entity to probe', 'warning');
    return;
  }

  await mesh.setSelectedEntity(entity);
  const now = Date.now();
  const record: ProbeRecord = {
    id: createId(),
    meshId: selectedMeshId.value,
    entity,
    createdAt: now,
    updatedAt: now
  };
  probes.value.unshift(record);
  activeProbeId.value = record.id;

  if (entity.propsInfo.size === 0) {
    setNotice('No properties loaded for this mesh', 'warning');
  } else {
    setNotice('Probe captured', 'success');
  }
};

const refreshProbe = async (record: ProbeRecord, silent = false) => {
  const mesh = getMeshById(record.meshId);
  if (!mesh) {
    if (!silent) {
      setNotice('Mesh is not available', 'warning');
    }
    return;
  }
  await mesh.setSelectedEntity(record.entity);
  const selected = mesh.getSelectedEntities();
  const updated = selected.find((item) =>
    item.entityId === record.entity.entityId &&
    item.entityType === record.entity.entityType &&
    item.lineIndex === record.entity.lineIndex
  );
  if (!updated) {
    if (!silent) {
      setNotice('Probe entity not found', 'warning');
    }
    return;
  }
  record.entity = updated;
  record.updatedAt = Date.now();
  if (!silent) {
    setNotice('Probe refreshed', 'success');
  }
};

const focusProbe = async (record: ProbeRecord) => {
  const mesh = getMeshById(record.meshId);
  if (!mesh) {
    setNotice('Mesh is not available', 'warning');
    return;
  }
  await mesh.setSelectedEntity(record.entity);
  const size = props.viewerService.getMeshBoxSize(record.meshId);
  const extent = Math.max(size.x, size.y, size.z);
  const pad = extent > 0 ? extent * 0.05 : 1;
  const pos = record.entity.pos;
  if (!Number.isFinite(pos.x) || !Number.isFinite(pos.y) || !Number.isFinite(pos.z)) {
    props.viewerService.fitAll(true);
    return;
  }
  props.viewerService.fitToBBox({
    cornerMin: { x: pos.x - pad, y: pos.y - pad, z: pos.z - pad },
    cornerMax: { x: pos.x + pad, y: pos.y + pad, z: pos.z + pad }
  }, true);
};

const removeProbe = (id: string) => {
  probes.value = probes.value.filter((probe) => probe.id !== id);
  if (activeProbeId.value === id) {
    activeProbeId.value = probes.value[0]?.id ?? '';
  }
};

const clearProbes = () => {
  probes.value = [];
  activeProbeId.value = '';
};

const serializeMap = (map: Map<any, any>) => Array.from(map.entries());

const exportJson = () => {
  if (probes.value.length === 0) {
    setNotice('No probe results to export', 'warning');
    return;
  }
  const payload = {
    activeId: activeProbeId.value || null,
    probes: probes.value.map((probe) => ({
      id: probe.id,
      meshId: probe.meshId,
      entity: {
        entityId: probe.entity.entityId,
        entityType: probe.entity.entityType,
        isElemSurf: probe.entity.isElemSurf,
        prsId: probe.entity.prsId,
        geomId: probe.entity.geomId,
        lineIndex: probe.entity.lineIndex,
        pos: probe.entity.pos
      },
      propsInfo: serializeMap(probe.entity.propsInfo),
      prsPropsInfo: serializeMap(probe.entity.prsPropsInfo),
      createdAt: probe.createdAt,
      updatedAt: probe.updatedAt
    }))
  };
  jsonText.value = JSON.stringify(payload, null, 2);
  setNotice('Probe JSON exported', 'success');
};

const copyJson = async () => {
  if (!jsonText.value) {
    setNotice('Nothing to copy', 'warning');
    return;
  }
  await navigator.clipboard.writeText(jsonText.value);
  setNotice('JSON copied', 'success');
};

const importProperties = async (files?: File[] | File | null) => {
  if (!files) {
    return;
  }
  const list = Array.isArray(files) ? files : [files];
  if (list.length === 0) {
    return;
  }
  const results = await props.viewerService.loadLocalFiles(list);
  const success = results.some((item) => item.success && item.kind === 'mesh-properties');
  setNotice(success ? 'Mesh properties loaded' : 'Failed to load properties', success ? 'success' : 'warning');
};

const handleSceneChanged = () => refreshMeshes();

let timeHandler: ((evt: Event) => void) | null = null;

onMounted(async () => {
  refreshMeshes();
  window.addEventListener('sceneChanged', handleSceneChanged as EventListener);
  await props.viewerService.whenReady();
  const viewer = props.viewerService.viewer;
  if (!viewer) {
    return;
  }
  timeHandler = (evt: Event) => {
    const e = evt as MeshAnimationChangedEvent;
    const probe = activeProbe.value;
    if (!probe || !e.mesh || e.mesh.id !== probe.meshId || !e.isChanged) {
      return;
    }
    void refreshProbe(probe, true);
  };
  viewer.eventManager.addEventListener(UserEvents.onMeshAnimationChanged, timeHandler);
});

onBeforeUnmount(() => {
  window.removeEventListener('sceneChanged', handleSceneChanged as EventListener);
  const viewer = props.viewerService.viewer;
  if (viewer && timeHandler) {
    viewer.eventManager.removeEventListener(UserEvents.onMeshAnimationChanged, timeHandler);
  }
});
</script>

<template>
  <v-card variant="tonal" class="cae-probe-panel" :disabled="!hasMesh">
    <v-card-title class="text-subtitle-2 d-flex align-center justify-space-between">
      <span>CAE Probe</span>
      <v-btn size="x-small" variant="text" @click="clearProbes" :disabled="probes.length === 0">
        Clear
      </v-btn>
    </v-card-title>
    <v-card-text class="probe-body">
      <v-select
        v-model="selectedMeshId"
        :items="meshes"
        item-title="label"
        item-value="id"
        label="Mesh instance"
        density="compact"
        hide-details
      />

      <div class="probe-actions">
        <v-btn size="x-small" variant="outlined" @click="pickProbe">Pick (hover)</v-btn>
        <v-btn
          size="x-small"
          variant="outlined"
          @click="activeProbe && refreshProbe(activeProbe)"
          :disabled="!activeProbe"
        >
          Refresh
        </v-btn>
      </div>

      <v-file-input
        density="compact"
        variant="outlined"
        hide-details
        label="Import property file"
        @update:modelValue="importProperties"
      />

      <v-alert v-if="notice" :type="notice.type" density="compact" variant="tonal" class="mt-2">
        {{ notice.text }}
      </v-alert>

      <div v-if="activeProbe" class="probe-summary">
        <div class="text-caption">Entity: {{ activeProbe.entity.entityId }} ({{ getEntityTypeLabel(activeProbe.entity.entityType) }})</div>
        <div class="text-caption">Position: {{ formatPos(activeProbe.entity.pos) }}</div>
        <div class="text-caption">Properties: {{ activeProbe.entity.propsInfo.size }}</div>
      </div>

      <div v-if="activeProbe && !hasProps" class="text-caption mt-2">
        No properties loaded for the active mesh. Import a property file to see values.
      </div>

      <v-list v-if="propertyEntries.length" density="compact" class="mt-2">
        <v-list-item v-for="entry in propertyEntries" :key="entry.propId">
          <v-list-item-title class="text-body-2">
            {{ entry.name }}
            <span class="text-caption">({{ entry.propId }})</span>
            <v-chip v-if="entry.isActive" size="x-small" class="ml-1" color="primary" variant="tonal">active</v-chip>
          </v-list-item-title>
          <v-list-item-subtitle class="text-caption">
            <div v-for="(comp, idx) in entry.info.components" :key="idx">
              {{ comp.description || `Component ${idx}` }}: {{ formatValue(comp.value) }} {{ comp.displayUnits }}
            </div>
          </v-list-item-subtitle>
          <template #append>
            <v-btn icon size="x-small" variant="text" @click.stop="focusProbe(activeProbe!)">
              <v-icon>mdi-crosshairs-gps</v-icon>
            </v-btn>
          </template>
        </v-list-item>
      </v-list>

      <v-divider class="my-2" />

      <v-textarea
        v-model="jsonText"
        density="compact"
        variant="outlined"
        rows="4"
        label="Probe JSON"
        hide-details
      />
      <div class="probe-actions">
        <v-btn size="x-small" variant="outlined" @click="exportJson">Export JSON</v-btn>
        <v-btn size="x-small" variant="text" @click="copyJson">Copy JSON</v-btn>
        <v-btn size="x-small" variant="text" @click="jsonText = ''">Clear JSON</v-btn>
      </div>

      <v-list v-if="probes.length" density="compact" class="mt-2">
        <v-list-item
          v-for="probe in probes"
          :key="probe.id"
          :active="probe.id === activeProbeId"
          @click="activeProbeId = probe.id"
        >
          <v-list-item-title class="text-body-2">
            {{ probe.meshId }} - {{ getEntityTypeLabel(probe.entity.entityType) }} {{ probe.entity.entityId }}
          </v-list-item-title>
          <v-list-item-subtitle class="text-caption">
            {{ new Date(probe.updatedAt).toLocaleTimeString() }}
          </v-list-item-subtitle>
          <template #append>
            <v-btn icon size="x-small" variant="text" @click.stop="focusProbe(probe)">
              <v-icon>mdi-crosshairs-gps</v-icon>
            </v-btn>
            <v-btn icon size="x-small" variant="text" @click.stop="removeProbe(probe.id)">
              <v-icon>mdi-delete</v-icon>
            </v-btn>
          </template>
        </v-list-item>
      </v-list>
    </v-card-text>
  </v-card>
</template>

<style scoped>
.cae-probe-panel {
  background: rgba(30, 32, 40, 0.9);
}

.probe-body {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.probe-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.probe-summary {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
</style>
