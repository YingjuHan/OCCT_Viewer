<script setup lang="ts">
import { PropType } from 'vue';
import Toolbar from '../Toolbar.vue';
import { ViewerService, LocalLoadResult } from '../../services/viewer.service';

const props = defineProps({
    viewerService: {
        type: Object as PropType<ViewerService>,
        required: true
    }
});

const emit = defineEmits<{
    (e: 'filesLoaded', results: LocalLoadResult[]): void
    (e: 'toggleRight'): void
}>();

const forwardFilesLoaded = (results: LocalLoadResult[]) => emit('filesLoaded', results);
</script>

<template>
    <v-app-bar flat class="lwv-app-bar" color="surface">
        <div class="brand-block">
            <div class="brand-title">OCCT LWV</div>
            <div class="brand-caption">Light Web Viewer</div>
        </div>
        <v-btn variant="text" class="text-none" rounded="pill">Demo</v-btn>
        <v-btn variant="text" class="text-none" rounded="pill">Docs</v-btn>
        <v-spacer />
        <Toolbar :viewerService="viewerService" @filesLoaded="forwardFilesLoaded" />
        <v-btn icon variant="text" class="ms-3" @click="emit('toggleRight')">
            <v-icon>mdi-dock-right</v-icon>
        </v-btn>
    </v-app-bar>
</template>

<style scoped>
.lwv-app-bar {
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
}

.brand-block {
    display: flex;
    flex-direction: column;
    margin-right: 24px;
    line-height: 1.1;
}

.brand-title {
    font-weight: 600;
    letter-spacing: 0.04em;
}

.brand-caption {
    font-size: 0.72rem;
    color: rgba(255, 255, 255, 0.65);
}
</style>
