<script setup lang="ts">
import { PropType } from 'vue';
import { ViewerService } from '../services/viewer.service';

const props = defineProps({
    viewerService: {
        type: Object as PropType<ViewerService>,
        required: true
    }
});

const run = (callback: () => void | Promise<void>) => {
    Promise.resolve(callback()).catch(err => console.error(err));
};
</script>

<template>
    <div class="tool-dock">
        <v-card variant="tonal">
            <v-card-title class="text-subtitle-2">快速导入</v-card-title>
            <v-card-text>
                <v-btn
                    block
                    color="primary"
                    variant="outlined"
                    class="mb-2"
                    @click="run(() => props.viewerService.loadShapeUrlAsync('models/brep/face.brep'))"
                >
                    加载示例几何
                </v-btn>
                <v-btn
                    block
                    color="secondary"
                    variant="outlined"
                    @click="run(() => props.viewerService.loadMeshUrlAsync('models/gmsh/buckling22.pos'))"
                >
                    加载示例网格
                </v-btn>
            </v-card-text>
        </v-card>
    </div>
</template>

<style scoped>
.tool-dock {
    display: flex;
    flex-direction: column;
    gap: 12px;
}
</style>
