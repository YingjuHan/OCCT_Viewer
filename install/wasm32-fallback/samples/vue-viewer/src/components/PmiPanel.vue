<script setup lang="ts">
import { computed, ref } from 'vue';
import { ViewerService } from '../services/viewer.service';

const props = defineProps<{
  viewerService: ViewerService;
}>();

const jsonText = ref('');
const notice = ref<{ text: string; type: 'info' | 'warning' | 'success' } | null>(null);

const store = computed(() => props.viewerService.pmiStore);
const items = computed(() => store.value?.items ?? []);

const setNotice = (text: string, type: 'info' | 'warning' | 'success' = 'info') => {
  notice.value = { text, type };
  setTimeout(() => {
    if (notice.value?.text === text) {
      notice.value = null;
    }
  }, 3000);
};

const importJson = () => {
  const res = props.viewerService.importPmiJson(jsonText.value);
  if (res.invalidCount > 0) {
    setNotice(`Imported with ${res.invalidCount} invalid items`, 'warning');
    return;
  }
  setNotice('PMI imported', 'success');
};

const clearPmi = () => {
  props.viewerService.clearPmi();
  setNotice('PMI cleared', 'info');
};

const focusItem = (id: string) => {
  const ok = store.value?.focus(id);
  if (!ok) {
    setNotice('Focus failed', 'warning');
  }
};

const importFile = async (file: File | null) => {
  if (!file) {
    return;
  }
  const text = await file.text();
  jsonText.value = text;
  importJson();
};
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2 d-flex align-center justify-space-between">
      <span>PMI (Read-only)</span>
      <v-btn size="x-small" variant="outlined" @click="clearPmi">Clear</v-btn>
    </v-card-title>

    <v-card-text class="pmi-panel">
      <v-file-input
        density="compact"
        variant="outlined"
        label="Import PMI JSON"
        accept=".json"
        hide-details
        @update:modelValue="(files) => importFile(files?.[0] ?? null)"
      />

      <v-alert v-if="notice" :type="notice.type" density="compact" variant="tonal" class="mt-2">
        {{ notice.text }}
      </v-alert>

      <v-list v-if="items.length" density="compact" class="mt-2">
        <v-list-item v-for="item in items" :key="item.id">
          <v-list-item-title class="text-body-2">
            {{ item.text || item.id }} <span v-if="!item.valid">(invalid)</span>
          </v-list-item-title>
          <v-list-item-subtitle class="text-caption">
            {{ item.type }} · {{ item.kind }} · {{ item.id }}
          </v-list-item-subtitle>
          <template #append>
            <v-btn icon size="x-small" variant="text" @click.stop="focusItem(item.id)">
              <v-icon>mdi-crosshairs-gps</v-icon>
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
        label="PMI JSON"
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
.pmi-panel {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.json-actions {
  display: flex;
  gap: 8px;
}
</style>
