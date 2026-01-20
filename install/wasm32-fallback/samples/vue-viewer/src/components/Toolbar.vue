<script lang="ts">
import { defineComponent, PropType, ref } from 'vue';
import { ViewerService, LocalLoadResult } from '../services/viewer.service';

export default defineComponent({
    name: 'Toolbar',
    props: {
        viewerService: {
            type: Object as PropType<ViewerService>,
            required: true
        }
    },
    emits: ['filesLoaded'],
    setup(props, { emit }) {
        const uploading = ref(false);

        const emitResults = (results: LocalLoadResult[]) => {
            if (results.length === 0) {
                return;
            }
            emit('filesLoaded', results);
        };

        const handleUpload = async (files: File[] | undefined) => {
            if (!files || files.length === 0) {
                return;
            }
            uploading.value = true;
            const results = await props.viewerService.loadLocalFiles(files);
            emitResults(results);
            uploading.value = false;
        };

        const handleFileChange = (files: File[] | undefined) => {
            void handleUpload(files);
        };

        return {
            uploading,
            handleFileChange,
            viewerService: props.viewerService
        };
    }
});
</script>

<template>
    <div class="command-bar">
        <v-file-input
            density="compact"
            hide-details
            multiple
            variant="solo-filled"
            prepend-inner-icon="mdi-upload"
            :loading="uploading"
            label="导入模型或网格"
            class="file-input"
            @update:modelValue="handleFileChange"
        />
        <v-divider vertical class="mx-4" />
        <v-btn-group variant="text" density="comfortable">
            <v-btn icon="mdi-fit-to-page-outline" @click="viewerService.fitAll(true)" :disabled="uploading" />
            <v-btn icon="mdi-autorenew" @click="viewerService.clearScene(true)" :disabled="uploading" />
            <v-btn icon="mdi-cube-outline" @click="viewerService.toggleViewCube()" :disabled="uploading" />
        </v-btn-group>
    </div>
</template>

<style scoped>
.command-bar {
    display: flex;
    align-items: center;
    gap: 12px;
    min-width: 420px;
}

.file-input {
    max-width: 260px;
}
</style>
