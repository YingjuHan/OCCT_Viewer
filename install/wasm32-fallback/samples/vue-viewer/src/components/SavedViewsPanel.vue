<script setup lang="ts">
import { computed, ref } from 'vue';
import { ViewerService } from '../services/viewer.service';

const props = defineProps<{
  viewerService: ViewerService;
}>();

const nameInput = ref('');
const jsonText = ref('');
const notice = ref<{ text: string; type: 'info' | 'warning' | 'success' } | null>(null);

const store = computed(() => props.viewerService.savedViewStore);
const views = computed(() => store.value?.views ?? []);

const setNotice = (text: string, type: 'info' | 'warning' | 'success' = 'info') => {
  notice.value = { text, type };
  setTimeout(() => {
    if (notice.value?.text === text) {
      notice.value = null;
    }
  }, 3000);
};

const saveView = () => {
  if (!store.value) return;
  const item = store.value.capture(nameInput.value.trim());
  if (!item) {
    setNotice('Save failed', 'warning');
    return;
  }
  nameInput.value = '';
  setNotice('View saved', 'success');
};

const applyView = (id: string) => {
  if (!store.value) return;
  const res = store.value.apply(id);
  if (res.missingIds.length > 0) {
    setNotice(`Applied with ${res.missingIds.length} missing objects`, 'warning');
  } else {
    setNotice('View applied', 'success');
  }
};

const removeView = (id: string) => {
  store.value?.remove(id);
};

const exportJson = () => {
  if (!store.value) return;
  jsonText.value = store.value.exportJson();
  setNotice('JSON exported', 'success');
};

const importJson = () => {
  if (!store.value) return;
  store.value.importJson(jsonText.value);
  setNotice('JSON imported', 'success');
};
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2 d-flex align-center justify-space-between">
      <span>Saved Views</span>
      <v-btn size="x-small" variant="outlined" @click="exportJson">Export JSON</v-btn>
    </v-card-title>

    <v-card-text class="saved-views-panel">
      <div class="save-actions">
        <v-text-field
          v-model="nameInput"
          density="compact"
          variant="outlined"
          label="View name"
          hide-details
        />
        <v-btn size="x-small" variant="outlined" @click="saveView">Save current view</v-btn>
      </div>

      <v-alert v-if="notice" :type="notice.type" density="compact" variant="tonal" class="mt-2">
        {{ notice.text }}
      </v-alert>

      <v-list v-if="views.length" density="compact" class="mt-2">
        <v-list-item v-for="view in views" :key="view.id">
          <v-list-item-title class="text-body-2">{{ view.name }}</v-list-item-title>
          <v-list-item-subtitle class="text-caption">{{ view.id }}</v-list-item-subtitle>
          <template #append>
            <v-btn icon size="x-small" variant="text" @click.stop="applyView(view.id)">
              <v-icon>mdi-eye</v-icon>
            </v-btn>
            <v-btn icon size="x-small" variant="text" @click.stop="removeView(view.id)">
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
        label="Saved Views JSON"
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
.saved-views-panel {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.save-actions {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.json-actions {
  display: flex;
  gap: 8px;
}
</style>
