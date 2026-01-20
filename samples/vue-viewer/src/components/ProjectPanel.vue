<script setup lang="ts">
import { ref, PropType } from 'vue';
import type { ProjectData, ProjectSource } from '../../../../source-ts/Viewer-ts/ProjectStore';
import type { ViewerService } from '../services/viewer.service';

const props = defineProps({
  viewerService: {
    type: Object as PropType<ViewerService>,
    required: true
  }
});

const message = ref<string | null>(null);
const missingSources = ref<ProjectSource[]>([]);
const lastProject = ref<ProjectData | null>(null);
const isBusy = ref(false);

const downloadText = (fileName: string, content: string) => {
  const blob = new Blob([content], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = fileName;
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  URL.revokeObjectURL(url);
};

const exportProject = () => {
  const json = props.viewerService.exportProject();
  if (!json) {
    message.value = 'No project data to export.';
    return;
  }
  downloadText('project.json', json);
  message.value = 'Project exported.';
};

const importProjectFile = async (file: File | null) => {
  if (!file) {
    return;
  }
  isBusy.value = true;
  message.value = null;
  try {
    const text = await file.text();
    const res = await props.viewerService.importProject(text);
    lastProject.value = res.project;
    missingSources.value = res.missingSources;
    message.value = res.message ?? (res.missingSources.length ? 'Project loaded. Rebind missing sources.' : 'Project loaded.');
  } finally {
    isBusy.value = false;
  }
};

const rebindMissing = async (src: ProjectSource, file: File | null) => {
  if (!file) {
    return;
  }
  await props.viewerService.rebindProjectSource(src.id, file);
  missingSources.value = missingSources.value.filter((s) => s.id !== src.id);
  message.value = missingSources.value.length ? 'Rebind remaining sources.' : 'Project ready.';
};
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2">Project</v-card-title>
    <v-card-text>
      <v-btn block color="primary" variant="outlined" class="mb-2" @click="exportProject">
        Export Project
      </v-btn>

      <v-file-input
        label="Import project.json"
        accept=".json"
        density="compact"
        :loading="isBusy"
        @update:modelValue="(files) => importProjectFile(files?.[0] ?? null)"
      />

      <div v-if="missingSources.length" class="mt-3">
        <div class="text-caption mb-2">Missing local sources</div>
        <div v-for="src in missingSources" :key="src.id" class="mb-2">
          <div class="text-caption">
            {{ src.id }} ({{ src.kind }} / {{ src.sourceType }})
          </div>
          <v-file-input
            density="compact"
            label="Rebind file"
            @update:modelValue="(files) => rebindMissing(src, files?.[0] ?? null)"
          />
        </div>
      </div>

      <v-alert v-if="message" type="info" density="compact" variant="tonal" class="mt-2">
        {{ message }}
      </v-alert>
    </v-card-text>
  </v-card>
</template>
