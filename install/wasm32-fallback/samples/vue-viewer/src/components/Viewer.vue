<script lang="ts">
import { defineComponent, onBeforeUnmount, onMounted, PropType, ref } from "vue"
import { ViewerService } from "../services/viewer.service";
import { SCwebviewer, MessageGravity, CameraOrientation } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
export default defineComponent({
  emits: ['filesLoaded'],
  props: {
    viewerService: {
        type: Object as PropType<ViewerService>,
        required: true
    }
  },
  setup(props, { emit }) {
    const isDragOver = ref(false);
    const containerRef = ref<HTMLElement | null>(null);
    let resizeObserver: ResizeObserver | null = null;

    const initViewer = async () => {
        await props.viewerService.createViewer();
        props.viewerService.onResize();
    };
    const resizeHandler = () => props.viewerService.onResize();

    const attachResizeObserver = () => {
        if (!containerRef.value) {
            return;
        }
        resizeObserver = new ResizeObserver(() => {
            props.viewerService.onResize();
        });
        resizeObserver.observe(containerRef.value);
    };

    const detachResizeObserver = () => {
        if (resizeObserver && containerRef.value) {
            resizeObserver.unobserve(containerRef.value);
        }
        resizeObserver = null;
    };

    const handleFileDrop = async (event: DragEvent) => {
        event.preventDefault();
        isDragOver.value = false;
        if (event.dataTransfer == null) {
            return;
        }

        const files: File[] = [];
        if (event.dataTransfer.items && event.dataTransfer.items.length > 0) {
            for (let i = 0; i < event.dataTransfer.items.length; ++i) {
                const item = event.dataTransfer.items[i];
                if (item.kind === 'file') {
                    const file = item.getAsFile();
                    if (file != null) {
                        files.push(file);
                    }
                }
            }
        } else if (event.dataTransfer.files) {
            files.push(...Array.from(event.dataTransfer.files));
        }

        if (files.length === 0) {
            return;
        }

        const results = await props.viewerService.loadLocalFiles(files);
        emit('filesLoaded', results);
    };

    const handleDragEnter = (event: DragEvent) => {
        event.preventDefault();
        isDragOver.value = true;
    };

    const handleDragLeave = (event: DragEvent) => {
        event.preventDefault();
        const currentTarget = event.currentTarget as HTMLElement;
        const relatedTarget = event.relatedTarget as Node | null;
        if (!currentTarget || relatedTarget == null) {
            isDragOver.value = false;
            return;
        }
        if (!currentTarget.contains(relatedTarget)) {
            isDragOver.value = false;
        }
    };

    const setOrientation = (orientation: CameraOrientation) => {
        switch (orientation) {
            case CameraOrientation.FRONT: props.viewerService.setFrontView(true); break;
            case CameraOrientation.BACK: props.viewerService.setBackView(true); break;
            case CameraOrientation.LEFT: props.viewerService.setLeftView(true); break;
            case CameraOrientation.RIGHT: props.viewerService.setRightView(true); break;
            case CameraOrientation.TOP: props.viewerService.setTopView(true); break;
            case CameraOrientation.BOTTOM: props.viewerService.setBottomView(true); break;
            case CameraOrientation.AXORIGHT: props.viewerService.setAxoView(true); break;
        }
    };

    onMounted(async () => {
        await initViewer();
        attachResizeObserver();
        window.addEventListener('resize', resizeHandler);
    });

    onBeforeUnmount(() => {
        detachResizeObserver();
        window.removeEventListener('resize', resizeHandler);
    });

    return {
        isDragOver,
        containerRef,
        handleFileDrop,
        handleDragEnter,
        handleDragLeave,
        setOrientation,
        CameraOrientation
    };
  }
})
</script>

<template>
    <div class="viewer-container"
         ref="containerRef"
         @dragenter.prevent="handleDragEnter"
         @dragover.prevent
         @dragleave.prevent="handleDragLeave"
         @drop.prevent="handleFileDrop">
        <canvas id="ViewerCanvas" class="webviewer-canvas" oncontextmenu="event.preventDefault()" tabindex=-1></canvas>
        <div class="viewer-drop-hint" v-if="isDragOver">
            <p>松开鼠标以加载本地 CAD/CAE 文件</p>
        </div>
        <div class="view-orbit">
            <v-btn icon size="small" variant="text" @click="setOrientation(CameraOrientation.TOP)">
                <v-icon>mdi-axis-z-arrow</v-icon>
            </v-btn>
            <v-btn icon size="small" variant="text" @click="setOrientation(CameraOrientation.FRONT)">
                <v-icon>mdi-axis-x-arrow</v-icon>
            </v-btn>
            <v-btn icon size="small" variant="text" @click="setOrientation(CameraOrientation.RIGHT)">
                <v-icon>mdi-axis-y-arrow</v-icon>
            </v-btn>
            <v-btn icon size="small" variant="text" @click="setOrientation(CameraOrientation.AXORIGHT)">
                <v-icon>mdi-rotate-3d-variant</v-icon>
            </v-btn>
        </div>
    </div>
</template>

<style scoped>
.viewer-container {
    position: relative;
    width: 100%;
    height: 100%;
    min-height: 480px;
}

.webviewer-canvas {
    min-width: 400px;
    min-height: 400px;
    user-select: none;
    border: 0;
    background-color: #000;
    display:block;
}

.viewer-drop-hint {
    position: absolute;
    inset: 20%;
    border: 2px dashed rgba(255, 255, 255, 0.5);
    border-radius: 12px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #fff;
    font-size: 16px;
    background: rgba(0, 0, 0, 0.5);
    pointer-events: none;
}

.view-orbit {
    position: absolute;
    top: 20px;
    right: 20px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    background: rgba(12, 12, 16, 0.75);
    border-radius: 12px;
    padding: 8px;
    box-shadow: 0 12px 24px rgba(0, 0, 0, 0.4);
}
</style>
