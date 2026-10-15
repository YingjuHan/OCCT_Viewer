<script setup lang="ts">
import { computed, ref, PropType } from 'vue';
import type { IShapeMassProps } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import { ShapeMassProps } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import type { ViewerService } from '../services/viewer.service';

const props = defineProps({
  viewerService: {
    type: Object as PropType<ViewerService>,
    required: true
  }
});

const rawJson = ref('');
const massProps = ref<IShapeMassProps | null>(null);
const notice = ref<{ type: 'info' | 'warning' | 'error'; text: string } | null>(null);

const selectedShapeId = computed(() => props.viewerService.getSelectedShape()?.id ?? '');

const formatLength = (value: number): string => {
  const unit = props.viewerService.unitManager?.length;
  return unit ? unit.format(value) : value.toString();
};

const formatArea = (value: number): string => {
  const unit = props.viewerService.unitManager?.area;
  return unit ? unit.format(value) : value.toString();
};

const formatVolume = (value: number): string => {
  const unit = props.viewerService.unitManager?.volume;
  return unit ? unit.format(value) : value.toString();
};

const compute = () => {
  notice.value = null;
  const shape = props.viewerService.getSelectedShape();
  if (!shape) {
    notice.value = { type: 'warning', text: 'Select a shape to compute mass properties.' };
    return;
  }

  const json = props.viewerService.getMassPropsJson(shape.id);
  rawJson.value = json;
  massProps.value = ShapeMassProps.parseJson(json);
  if (!massProps.value) {
    notice.value = { type: 'error', text: 'Failed to parse mass properties.' };
  }
};

const copyJson = async () => {
  if (!rawJson.value) {
    notice.value = { type: 'info', text: 'Nothing to copy yet.' };
    return;
  }
  await navigator.clipboard.writeText(rawJson.value);
  notice.value = { type: 'info', text: 'Mass properties copied.' };
};
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2">Mass Properties</v-card-title>
    <v-card-text>
      <div class="text-caption mb-2">
        Selected: <strong>{{ selectedShapeId || 'None' }}</strong>
      </div>
      <v-btn block color="primary" variant="outlined" class="mb-2" @click="compute">
        Compute
      </v-btn>
      <v-btn block variant="text" @click="copyJson">
        Copy JSON
      </v-btn>

      <div v-if="massProps" class="mt-3 text-body-2">
        <div>Area: {{ formatArea(massProps.area) }}</div>
        <div>
          Volume:
          <span v-if="massProps.volumeValid && massProps.volume !== null">
            {{ formatVolume(massProps.volume) }}
          </span>
          <span v-else>n/a (open shape)</span>
        </div>
        <div>
          Centroid:
          {{ formatLength(massProps.centroid.x) }},
          {{ formatLength(massProps.centroid.y) }},
          {{ formatLength(massProps.centroid.z) }}
          ({{ massProps.centroidSource }})
        </div>
      </div>

      <v-alert v-if="notice" :type="notice.type" density="compact" variant="tonal" class="mt-2">
        {{ notice.text }}
      </v-alert>
    </v-card-text>
  </v-card>
</template>
