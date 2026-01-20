<script setup lang="ts">
import { onBeforeUnmount, onMounted, ref, PropType } from 'vue';
import { MessageGravity, UserEvents } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import type { ViewerMessageEvent } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import type { ViewerService } from '../services/viewer.service';

const props = defineProps({
  viewerService: {
    type: Object as PropType<ViewerService>,
    required: true
  }
});

type ToastItem = {
  message: string;
  rawMessage: string;
  gravity: number;
  errorCode: string | null;
};

const queue = ref<ToastItem[]>([]);
const current = ref<ToastItem | null>(null);
const isOpen = ref(false);

const gravityToType = (gravity: number) => {
  if (gravity >= MessageGravity.Alarm) {
    return 'error';
  }
  if (gravity >= MessageGravity.Warning) {
    return 'warning';
  }
  return 'info';
};

const showNext = () => {
  if (queue.value.length === 0) {
    current.value = null;
    isOpen.value = false;
    return;
  }
  current.value = queue.value.shift() ?? null;
  isOpen.value = !!current.value;
};

const copyDetails = async () => {
  if (!current.value) {
    return;
  }
  const payload = {
    errorCode: current.value.errorCode ?? undefined,
    message: current.value.message,
    rawMessage: current.value.rawMessage,
    gravity: current.value.gravity
  };
  await navigator.clipboard.writeText(JSON.stringify(payload, null, 2));
};

let handler: ((evt: Event) => void) | null = null;

onMounted(async () => {
  await props.viewerService.whenReady();
  const viewer = props.viewerService.viewer;
  if (!viewer) {
    return;
  }
  handler = (evt: Event) => {
    const msg = evt as ViewerMessageEvent;
    queue.value.push({
      message: msg.message,
      rawMessage: msg.rawMessage,
      gravity: msg.gravity,
      errorCode: msg.errorCode
    });
    if (!isOpen.value) {
      showNext();
    }
  };
  viewer.eventManager.addEventListener(UserEvents.onViewerMessage, handler);
});

onBeforeUnmount(() => {
  if (handler && props.viewerService.viewer) {
    props.viewerService.viewer.eventManager.removeEventListener(UserEvents.onViewerMessage, handler);
  }
});
</script>

<template>
  <v-snackbar
    v-model="isOpen"
    :timeout="5000"
    location="top right"
    @update:modelValue="(val) => { if (!val) { showNext(); } }"
  >
    <v-alert v-if="current" :type="gravityToType(current.gravity)" density="compact" variant="tonal">
      <div class="text-body-2">{{ current.message || 'Viewer message' }}</div>
      <div v-if="current.errorCode" class="text-caption mt-1">Code: {{ current.errorCode }}</div>
      <div class="mt-2">
        <v-btn size="x-small" variant="text" @click="copyDetails">Copy</v-btn>
      </div>
    </v-alert>
  </v-snackbar>
</template>
