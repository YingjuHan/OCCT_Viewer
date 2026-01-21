<script setup lang="ts">
import { ref, PropType } from 'vue';
import type { IInterferenceResult, ObjectId } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import { InterferenceResult, InteractiveShape } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import type { ViewerService } from '../services/viewer.service';

type InterferenceRecord = {
  id: string;
  result: IInterferenceResult;
  tolerance: number;
  createdAt: number;
};

const props = defineProps({
  viewerService: {
    type: Object as PropType<ViewerService>,
    required: true
  }
});

const objA = ref<ObjectId>('');
const objB = ref<ObjectId>('');
const tolerance = ref(0);
const busy = ref(false);
const notice = ref<{ type: 'info' | 'warning' | 'error' | 'success'; text: string } | null>(null);
const records = ref<InterferenceRecord[]>([]);

const createId = () => {
  if (typeof crypto !== 'undefined' && 'randomUUID' in crypto) {
    return (crypto as Crypto).randomUUID();
  }
  return `interf-${Date.now().toString(16)}-${Math.random().toString(16).slice(2)}`;
};

const setNotice = (text: string, type: 'info' | 'warning' | 'error' | 'success' = 'info') => {
  notice.value = { text, type };
  setTimeout(() => {
    if (notice.value?.text === text) {
      notice.value = null;
    }
  }, 3000);
};

const getSelectedShapes = (): InteractiveShape[] => {
  const viewer = props.viewerService.viewer;
  if (!viewer?.selectionManager) {
    return [];
  }
  return viewer.selectionManager.selectedObjects.filter(
    (obj): obj is InteractiveShape => obj instanceof InteractiveShape
  );
};

const setA = () => {
  const shapes = getSelectedShapes();
  if (shapes.length === 0) {
    setNotice('Select a shape first', 'warning');
    return;
  }
  objA.value = shapes[0].id;
};

const setB = () => {
  const shapes = getSelectedShapes();
  if (shapes.length === 0) {
    setNotice('Select a shape first', 'warning');
    return;
  }
  objB.value = shapes[0].id;
};

const setFromSelection = () => {
  const shapes = getSelectedShapes();
  if (shapes.length < 2) {
    setNotice('Select at least two shapes', 'warning');
    return;
  }
  objA.value = shapes[0].id;
  objB.value = shapes[1].id;
};

const swapTargets = () => {
  const tmp = objA.value;
  objA.value = objB.value;
  objB.value = tmp;
};

const formatDistance = (distance: number | null): string => {
  if (distance == null) {
    return 'n/a';
  }
  const unit = props.viewerService.unitManager?.length;
  return unit ? unit.format(distance) : distance.toFixed(6);
};

const formatSummary = (record: InterferenceRecord): string => {
  const res = record.result;
  if (!res.bboxHit) {
    return 'AABB separated';
  }
  const distText = formatDistance(res.distance);
  return res.interferes ? `Interferes · d=${distText}` : `Clear · d=${distText}`;
};

const runCheck = () => {
  const viewer = props.viewerService.viewer;
  if (!viewer?.interactiveShapeManager) {
    setNotice('Viewer not ready', 'warning');
    return;
  }
  if (!objA.value || !objB.value) {
    setNotice('Select A and B first', 'warning');
    return;
  }
  if (objA.value === objB.value) {
    setNotice('A and B cannot be the same object', 'warning');
    return;
  }

  const tol = Number.isFinite(tolerance.value) ? tolerance.value : 0;
  busy.value = true;
  try {
    const json = viewer.interactiveShapeManager.checkInterferenceJson(objA.value, objB.value, tol);
    const parsed = InterferenceResult.parseJson(json);
    if (!parsed) {
      setNotice('Failed to parse interference result', 'error');
      return;
    }
    records.value.unshift({
      id: createId(),
      result: parsed,
      tolerance: tol,
      createdAt: Date.now()
    });
    setNotice('Interference check done', 'success');
  } finally {
    busy.value = false;
  }
};

const focusRecord = (record: InterferenceRecord) => {
  const viewer = props.viewerService.viewer;
  if (!viewer?.selectionManager) {
    return;
  }
  const obj1 = props.viewerService.getInteractiveObject(record.result.objId1);
  const obj2 = props.viewerService.getInteractiveObject(record.result.objId2);
  const selection = [obj1, obj2].filter((obj): obj is InteractiveShape => !!obj);
  if (selection.length === 0) {
    setNotice('Objects no longer in scene', 'warning');
    return;
  }
  viewer.selectionManager.selectedObjects = selection;
  viewer.camera?.fitAllAuto(true, true);
  viewer.requestViewUpdate();
};

const removeRecord = (id: string) => {
  records.value = records.value.filter((item) => item.id !== id);
};

const clearAll = () => {
  records.value = [];
};
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2 d-flex align-center justify-space-between">
      <span>Interference Check</span>
      <v-btn size="x-small" variant="text" @click="clearAll" :disabled="records.length === 0">Clear</v-btn>
    </v-card-title>
    <v-card-text class="interference-panel">
      <v-text-field
        v-model="objA"
        density="compact"
        variant="outlined"
        label="Object A"
        hide-details
        readonly
      />
      <v-text-field
        v-model="objB"
        density="compact"
        variant="outlined"
        label="Object B"
        hide-details
        readonly
      />
      <div class="interference-actions">
        <v-btn size="x-small" variant="outlined" @click="setA">Set A</v-btn>
        <v-btn size="x-small" variant="outlined" @click="setB">Set B</v-btn>
        <v-btn size="x-small" variant="outlined" @click="setFromSelection">Use 2 Selected</v-btn>
        <v-btn size="x-small" variant="text" @click="swapTargets">Swap</v-btn>
      </div>
      <v-text-field
        v-model.number="tolerance"
        type="number"
        density="compact"
        variant="outlined"
        label="Tolerance (0 = auto)"
        hide-details
      />
      <v-btn block color="primary" variant="outlined" :loading="busy" class="mt-2" @click="runCheck">
        Run Check
      </v-btn>

      <v-alert v-if="notice" :type="notice.type" density="compact" variant="tonal" class="mt-2">
        {{ notice.text }}
      </v-alert>

      <v-list v-if="records.length" density="compact" class="mt-2">
        <v-list-item v-for="item in records" :key="item.id">
          <v-list-item-title class="text-body-2">
            {{ item.result.objId1 }} <-> {{ item.result.objId2 }}
          </v-list-item-title>
          <v-list-item-subtitle class="text-caption">
            {{ formatSummary(item) }}
          </v-list-item-subtitle>
          <template #append>
            <v-btn icon size="x-small" variant="text" @click.stop="focusRecord(item)">
              <v-icon>mdi-crosshairs-gps</v-icon>
            </v-btn>
            <v-btn icon size="x-small" variant="text" @click.stop="removeRecord(item.id)">
              <v-icon>mdi-delete</v-icon>
            </v-btn>
          </template>
        </v-list-item>
      </v-list>
    </v-card-text>
  </v-card>
</template>

<style scoped>
.interference-panel {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.interference-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}
</style>
