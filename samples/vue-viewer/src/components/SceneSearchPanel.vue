<script setup lang="ts">
import { computed, ref, PropType } from 'vue';
import type { ViewerService } from '../services/viewer.service';

const props = defineProps({
  viewerService: {
    type: Object as PropType<ViewerService>,
    required: true
  }
});

const query = ref('');

const results = computed(() => {
  if (!query.value.trim()) {
    return [];
  }
  return props.viewerService.searchScene(query.value.trim());
});

const focus = (id: string) => {
  props.viewerService.selectObjectById(id);
  const obj = props.viewerService.getInteractiveObject(id);
  if (obj) {
    props.viewerService.fitToBBox(obj.minMax, true);
  }
};

const isolate = (id: string) => {
  props.viewerService.isolateObjectIds([id]);
};
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2">Scene Search</v-card-title>
    <v-card-text>
      <v-text-field
        v-model="query"
        label="Search by name or id"
        density="compact"
        hide-details
      />

      <v-list v-if="results.length" density="compact" class="mt-2">
        <v-list-item v-for="item in results" :key="item.id">
          <v-list-item-title>{{ item.name || item.id }}</v-list-item-title>
          <v-list-item-subtitle>
            {{ item.id }} · {{ item.matchField }}
          </v-list-item-subtitle>
          <template #append>
            <v-btn size="x-small" variant="text" @click="focus(item.id)">Fit</v-btn>
            <v-btn size="x-small" variant="text" @click="isolate(item.id)">Isolate</v-btn>
          </template>
        </v-list-item>
      </v-list>

      <div v-else class="text-caption mt-2">No matches.</div>
    </v-card-text>
  </v-card>
</template>
