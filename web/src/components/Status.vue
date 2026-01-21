<template>
    <div>
        <dl>
            <dt>PC:</dt>
            <dd>{{ pc }}</dd>
            <dd>0x{{ pc.toString(16).padStart(4, "0") }}</dd>
            <dd class="list">{{ list && list.getLine(pc) }}</dd>
            <dt>WP:</dt>
            <dd>{{ wp }}</dd>
            <dd>0x{{ wp.toString(16).padStart(4, "0") }}</dd>
            <dt>Status:</dt>
            <dd>{{ st.toString(2).padStart(16, "0") }}</dd>
            <dt>Cycles:</dt>
            <dd>{{ cycles.toLocaleString() }}</dd>
        </dl>
    </div>
</template>

<script setup>
import { defineProps, ref, onMounted, onUnmounted } from 'vue'
import { List } from '@ue-tms9900/emulator/util/List';

const props = defineProps(['cpu'])
const pc = ref(0);
const wp = ref(0);
const st = ref(0);
const cycles = ref(0);
const list = ref(false);

onMounted(async () => {
    list.value = new List(await (await fetch("hellorld.lst")).text());
    console.log(list.value);
});

function update() {
    let cpu = props.cpu;
    if (cpu) {
        pc.value = cpu.getPc();
        wp.value = cpu.getWp();
        st.value = cpu.getSt();
        cycles.value = cpu.getCycles();
    }
    requestAnimationFrame(update);
}
requestAnimationFrame(update);

</script>

<style scoped>
    .list {
        font-family: monospace;
        white-space: pre;
    }
</style>