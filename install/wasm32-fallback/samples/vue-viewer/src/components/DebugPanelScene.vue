<script lang="ts">

import { defineComponent, PropType } from "vue"
import { ViewerService } from "../services/viewer.service";
import { CaeMeshObjectPrsMode } from '@hw/SCwebviewer/wasm32/SCwebviewer.js';

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
        fitAll() {
            this.viewerService.fitAll(true);
        },

        setFrontView() {
            this.viewerService.setFrontView(true);
        },

        setBackView() {
            this.viewerService.setBackView(true);
        },

        setLeftView() {
            this.viewerService.setLeftView(true);
        },

        setRightView() {
            this.viewerService.setRightView(true);
        },

        setTopView() {
            this.viewerService.setTopView(true);
        },

        setBottomView() {
            this.viewerService.setBottomView(true);
        },

        setAxoView() {
            this.viewerService.setAxoView(true);
        },

        clearScene() {
            this.viewerService.clearScene(true);
        }
    }
})

</script>

<template>
    <div class="debug">
        <p>Scene</p>
        <button @click="fitAll">Fit All</button>
        <button @click="setFrontView">Front View</button>
        <button @click="setBackView">Back View</button>
        <button @click="setLeftView">Left View</button>
        <button @click="setRightView">Right View</button>
        <button @click="setTopView">Top View</button>
        <button @click="setBottomView">Bottom View</button>
        <button @click="setAxoView">Axo View</button>
        <button @click="clearScene">Clear Scene</button>
    </div>
</template>

<style scoped>
.debug {
    display: flex;
    flex-direction: column;
}
</style>