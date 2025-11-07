<script lang="ts">
import { defineComponent, PropType } from "vue"
import { ViewerService } from "../services/viewer.service";
import { SCwebviewer, MessageGravity, CameraOrientation } from '@hw/SCwebviewer/wasm32/SCwebviewer.js';
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
    async initViewer(): Promise<void> {
        await this.viewerService.createViewer();

        this.viewerService.onResize();
    }
  },
  mounted() {
    this.initViewer();
  }
})
</script>

<template>
    <div>
        <canvas id="ViewerCanvas" class="webviewer-canvas" oncontextmenu="event.preventDefault()" tabindex=-1></canvas>
    </div>
</template>

<style scoped>
.webviewer-canvas {
    min-width: 400px;
    min-height: 400px;
    user-select: none;
    border: 0;
    background-color: #000;
    display:block;
}
</style>