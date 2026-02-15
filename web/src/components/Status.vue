<template>
    <div class="scope">
        <div>
            <h2>Status:</h2>
            <dl>
                <dt>PC:</dt>
                <dd>0x{{ pc.toString(16).padStart(4, "0") }}</dd>
                <dt>WP:</dt>
                <dd>0x{{ wp.toString(16).padStart(4, "0") }}</dd>
                <dt>Status:</dt>
                <dd>{{ st.toString(2).padStart(16, "0") }}</dd>
                <dd>
                    <StatusRegister v-model="st" />
                </dd>
                <dt>Cycles:</dt>
                <dd>{{ cycles.toLocaleString() }}</dd>
                <dt>Interrupts:</dt>
                <dd><span v-if="int !== false">{{ int.toString(2).padStart(4, "0") }}</span><span v-else>none</span>
                </dd>
            </dl>
        </div>
        <div>
            <h2>Registers:</h2>
            <dl class="registers">
                <template v-for="r in 16" :key="r">
                    <dt>R{{ r }}</dt>
                    <dd>0x{{ ue.cpu.getMemoryWord(wp + 2 * r).toString(16).padStart(4, "0") }}</dd>
                </template>
            </dl>
        </div>
        <div>
            <h2>CPU Time:</h2>
            <dl>
                <template v-for="(pct, wp) in pct" :key="wp">
                    <dt>0x{{ parseInt(wp).toString(16).padStart(4, "0") }}</dt>
                    <dd><progress max="100" :value="pct"></progress></dd>
                </template>
            </dl>
        </div>
    </div>
</template>

<script setup>
import { defineProps, ref, onMounted, onUnmounted } from 'vue'
import StatusRegister from './StatusRegister.vue';

const props = defineProps(['ue']);
const pc = ref(0);
const wp = ref(0);
const st = ref(0);
const cycles = ref(0);
const int = ref({});
const pct = ref({});

function update() {
    let cpu = props.ue.cpu;
    if (cpu) {
        pc.value = cpu.getPc();
        wp.value = cpu.getWp();
        st.value = cpu.getSt();
        cycles.value = cpu.getCycles();
        int.value = props.ue.intEnc.getInterruptState();
        let wpProfile = cpu.getWpProfile();
        let wpTotal = 0;
        for (const [wp, cycles] of Object.entries(wpProfile)) {
            wpTotal += cycles;
        }
        for (const [wp, cycles] of Object.entries(wpProfile)) {
            pct.value[wp] = 100 * cycles / wpTotal;
        }
    }
    requestAnimationFrame(update);
}
requestAnimationFrame(update);

</script>

<style scoped>
div.scope {
   
}

div.scope>div {
    display: inline-block;
    vertical-align: top;
}

.list {
    font-family: monospace;
    white-space: pre;
}

dl {
    display: grid;
    grid-template-columns: max-content auto;
}

dt {
    font-weight: bold;
    grid-column-start: 1;
    margin-bottom: 5px;
}

dd {
    grid-column-start: 2;
    margin-bottom: 5px;
}

dl.registers {
    grid-template-columns: max-content auto max-content auto;
    width: 400px;
}

dl.registers dt {
    grid-column-start: unset;
}

dl.registers dd {
    grid-column-start: unset;
}

progress {
    accent-color: rgb(0, 210, 0);
}
</style>