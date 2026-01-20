<script setup lang="ts">
import { PropType } from 'vue';
import type { QuickAction } from '../../constants/quick-actions';

const props = defineProps({
    actions: {
        type: Array as PropType<QuickAction[]>,
        required: true
    }
});

const emit = defineEmits<{
    (e: 'trigger', action: string): void
}>();

const handleClick = (action: string) => emit('trigger', action);
</script>

<template>
    <div class="quick-rail elevation-3">
        <v-btn
            v-for="item in props.actions"
            :key="item.action"
            :title="item.label"
            icon
            color="primary"
            variant="text"
            size="small"
            @click="handleClick(item.action)"
        >
            <v-icon>{{ item.icon }}</v-icon>
        </v-btn>
        <slot></slot>
    </div>
</template>

<style scoped>
.quick-rail {
    width: 60px;
    border-radius: 18px;
    padding: 12px 8px;
    display: flex;
    flex-direction: column;
    gap: 8px;
    background: rgba(22, 25, 32, 0.85);
}
</style>
