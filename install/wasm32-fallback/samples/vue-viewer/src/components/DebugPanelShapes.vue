<script lang="ts">

import { defineComponent, PropType } from "vue"
import { ViewerService } from "../services/viewer.service";
import { CaeMeshObjectPrsMode } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';

export default defineComponent({
    props: {
        viewerService: {
            type: Object as PropType<ViewerService>,
            required: true
        }
    },
    data() {
        return {
        }
    },
    methods: {
        loadShapeTestModel() {
            this.viewerService.loadShapeUrlAsync("models/gltf/as1-oc-214.glb").then((result) => {
                if (result) {
                    this.viewerService.fitAll(true);
                    console.log("Model has been loaded");
                }
            });
        }
    }
})

</script>

<template>
    <div class="debug">
        <p>Shapes</p>
        <button @click="loadShapeTestModel">Load Shape Test Model</button>
    </div>
</template>

<style scoped>
.debug {
    display: flex;
    flex-direction: column;
}
</style>