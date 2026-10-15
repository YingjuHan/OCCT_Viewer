<script setup lang="ts">
import { computed, ref } from 'vue';
import { ViewerService } from '../services/viewer.service';
import { InteractiveShape, ShapeObjectSelMode } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';

type MeasurementTarget = {
  objId: string;
  selMode: number;
  entityId: number;
};

const props = defineProps<{
  viewerService: ViewerService;
}>();

const targetA = ref<MeasurementTarget | null>(null);
const targetB = ref<MeasurementTarget | null>(null);
const jsonText = ref('');
const notice = ref<{ text: string; type: 'info' | 'warning' | 'success' } | null>(null);

const store = computed(() => props.viewerService.measurementStore);
const items = computed(() => store.value?.items ?? []);

const formatTarget = (t: MeasurementTarget | null) => {
  if (!t) return '';
  return `${t.objId} [mode=${t.selMode}, id=${t.entityId}]`;
};

const setNotice = (text: string, type: 'info' | 'warning' | 'success' = 'info') => {
  notice.value = { text, type };
  setTimeout(() => {
    if (notice.value?.text === text) {
      notice.value = null;
    }
  }, 3000);
};

const captureTarget = (): MeasurementTarget | null => {
  const viewer = props.viewerService.viewer;
  const selMgr = viewer?.selectionManager;
  if (!viewer || !selMgr) {
    return null;
  }
  const detObj = selMgr.lastDetectedObject;
  if (!detObj || !(detObj instanceof InteractiveShape)) {
    return null;
  }
  const elem = selMgr.getDetectedShapeElem(detObj.id);
  if (!elem || typeof elem.entityId !== 'number') {
    return null;
  }
  return { objId: detObj.id, selMode: elem.entityType, entityId: elem.entityId };
};

const setA = () => {
  const t = captureTarget();
  if (!t) {
    setNotice('Set A: hover a shape element first', 'warning');
    return;
  }
  targetA.value = t;
};

const setB = () => {
  const t = captureTarget();
  if (!t) {
    setNotice('Set B: hover a shape element first', 'warning');
    return;
  }
  targetB.value = t;
};

const addDistance = () => {
  if (!store.value) return;
  if (!targetA.value || !targetB.value) {
    setNotice('Select A and B first', 'warning');
    return;
  }
  const item = store.value.addDistance(targetA.value, targetB.value);
  if (!item) {
    setNotice('Distance failed', 'warning');
    return;
  }
  setNotice('Distance added', 'success');
};

const addRadius = () => {
  if (!store.value) return;
  const t = captureTarget();
  if (!t) {
    setNotice('Radius: hover an edge/face first', 'warning');
    return;
  }
  if (t.selMode !== ShapeObjectSelMode.Edge && t.selMode !== ShapeObjectSelMode.Face) {
    setNotice('Radius: only edge or face is supported', 'warning');
    return;
  }
  const item = store.value.addRadius(t);
  if (!item) {
    setNotice('Radius failed', 'warning');
    return;
  }
  setNotice('Radius added', 'success');
};

const addDiameter = () => {
  if (!store.value) return;
  const t = captureTarget();
  if (!t) {
    setNotice('Diameter: hover an edge/face first', 'warning');
    return;
  }
  if (t.selMode !== ShapeObjectSelMode.Edge && t.selMode !== ShapeObjectSelMode.Face) {
    setNotice('Diameter: only edge or face is supported', 'warning');
    return;
  }
  const item = store.value.addDiameter(t);
  if (!item) {
    setNotice('Diameter failed', 'warning');
    return;
  }
  setNotice('Diameter added', 'success');
};

const clearTargets = () => {
  targetA.value = null;
  targetB.value = null;
};

const exportJson = () => {
  if (!store.value) return;
  jsonText.value = store.value.exportJson();
  setNotice('JSON exported', 'success');
};

const importJson = () => {
  if (!store.value) return;
  const res = store.value.importJson(jsonText.value);
  if (res.invalidCount > 0) {
    setNotice(`Imported with ${res.invalidCount} invalid items`, 'warning');
    return;
  }
  setNotice('JSON imported', 'success');
};

const focusItem = (id: string) => {
  if (!store.value) return;
  const ok = store.value.focus(id);
  if (!ok) {
    setNotice('Focus failed', 'warning');
  }
};

const removeItem = (id: string) => {
  store.value?.remove(id);
};
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2 d-flex align-center justify-space-between">
      <span>Measurements</span>
      <v-btn size="x-small" variant="outlined" @click="exportJson">Export JSON</v-btn>
    </v-card-title>

    <v-card-text class="measurement-panel">
      <div class="measure-actions">
        <v-text-field
          :model-value="formatTarget(targetA)"
          density="compact"
          variant="outlined"
          label="A"
          hide-details
          readonly
        />
        <v-text-field
          :model-value="formatTarget(targetB)"
          density="compact"
          variant="outlined"
          label="B"
          hide-details
          readonly
        />
        <div class="measure-buttons">
          <v-btn size="x-small" variant="outlined" @click="setA">Set A (hover)</v-btn>
          <v-btn size="x-small" variant="outlined" @click="setB">Set B (hover)</v-btn>
          <v-btn size="x-small" variant="outlined" @click="addDistance">Distance</v-btn>
          <v-btn size="x-small" variant="outlined" @click="addRadius">Radius</v-btn>
          <v-btn size="x-small" variant="outlined" @click="addDiameter">Diameter</v-btn>
          <v-btn size="x-small" variant="text" @click="clearTargets">Clear A/B</v-btn>
        </div>
      </div>

      <v-alert v-if="notice" :type="notice.type" density="compact" variant="tonal" class="mt-2">
        {{ notice.text }}
      </v-alert>

      <v-list v-if="items.length" density="compact" class="mt-2">
        <v-list-item v-for="item in items" :key="item.id">
          <v-list-item-title class="text-body-2">
            {{ item.label }} <span v-if="!item.valid">(invalid)</span>
          </v-list-item-title>
          <v-list-item-subtitle class="text-caption">
            {{ item.valueText || item.value }} · {{ item.kind }} · {{ item.id }}
          </v-list-item-subtitle>
          <template #append>
            <v-btn icon size="x-small" variant="text" @click.stop="focusItem(item.id)">
              <v-icon>mdi-crosshairs-gps</v-icon>
            </v-btn>
            <v-btn icon size="x-small" variant="text" @click.stop="removeItem(item.id)">
              <v-icon>mdi-delete</v-icon>
            </v-btn>
          </template>
        </v-list-item>
      </v-list>

      <v-textarea
        v-model="jsonText"
        class="mt-2"
        density="compact"
        variant="outlined"
        rows="4"
        label="Measurement JSON"
        hide-details
      />
      <div class="json-actions">
        <v-btn size="x-small" variant="outlined" @click="importJson">Import JSON</v-btn>
        <v-btn size="x-small" variant="text" @click="jsonText = ''">Clear JSON</v-btn>
      </div>
    </v-card-text>
  </v-card>
</template>

<style scoped>
.measurement-panel {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.measure-actions {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.measure-buttons {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.json-actions {
  display: flex;
  gap: 8px;
}
</style>
