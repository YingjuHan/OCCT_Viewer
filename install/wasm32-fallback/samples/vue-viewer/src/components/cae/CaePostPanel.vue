<script setup lang="ts">
import { computed, PropType } from 'vue';
import { CaeMeshObjectPrsMode } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import type { ViewerService } from '../../services/viewer.service';
import { useCaePostProcess } from '../../composables/useCaePostProcess';

const props = defineProps({
    viewerService: {
        type: Object as PropType<ViewerService>,
        required: true
    }
});

const {
    meshes,
    selectedMeshId,
    meshProperties,
    selectedProperty,
    displayMode,
    elementStatus,
    deformationScale
} = useCaePostProcess(props.viewerService);

const displayModeOptions = computed(() => ([
    { label: 'Mesh Edges', value: CaeMeshObjectPrsMode.MeshEdges },
    { label: 'Flat Shaded', value: CaeMeshObjectPrsMode.FlatShaded },
    { label: 'Edges + Shaded', value: CaeMeshObjectPrsMode.MeshEdgesFlatShaded },
    { label: 'Shrink', value: CaeMeshObjectPrsMode.MeshShrinked }
]));

const hasMesh = computed(() => meshes.value.length > 0);
</script>

<template>
    <v-card variant="tonal" class="cae-panel" :disabled="!hasMesh">
        <v-card-title class="text-subtitle-2">CAE 后处理</v-card-title>
        <v-card-text>
            <v-select
                v-model="selectedMeshId"
                :items="meshes"
                item-title="label"
                item-value="id"
                label="网格实例"
                density="compact"
                hide-details
            />
            <v-select
                v-model="selectedProperty"
                :items="meshProperties"
                item-title="name"
                item-value="propId"
                label="标量属性"
                density="compact"
                class="mt-3"
                hide-details
                :disabled="meshProperties.length === 0"
            />
            <v-select
                v-model="displayMode"
                :items="displayModeOptions"
                item-title="label"
                item-value="value"
                label="显示模式"
                density="compact"
                class="mt-3"
                hide-details
            />
            <v-divider class="my-3" />
            <v-switch
                v-model="elementStatus.elements3d"
                label="显示3D单元"
                color="primary"
                density="compact"
                hide-details
            />
            <v-switch
                v-model="elementStatus.elements2d"
                label="显示2D单元"
                color="primary"
                density="compact"
                hide-details
            />
            <v-switch
                v-model="elementStatus.nodes"
                label="显示节点"
                color="primary"
                density="compact"
                hide-details
            />
            <v-divider class="my-3" />
            <v-slider
                v-model="deformationScale"
                label="变形放大倍数"
                color="primary"
                min="0"
                max="10"
                step="0.1"
                thumb-label
            />
        </v-card-text>
    </v-card>
</template>

<style scoped>
.cae-panel {
    background: rgba(30, 32, 40, 0.9);
}
</style>
