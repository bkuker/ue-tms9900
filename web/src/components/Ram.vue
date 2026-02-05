<template>
    <div class="ram">
        <h2>RAM</h2>
        <dl>
            <template v-for="(sym, addr) in addrToSym">
                <dt>0x{{ parseInt(addr).toString(16).padStart(4, "0") }} {{ sym }}:</dt>
                <dd>0x{{ values[addr]?.toString(16).padStart(4, "0")  }}</dd>
            </template>
        </dl>
    </div>
</template>
<script setup>
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { Memory } from '@ue-tms9900/emulator/Memory';
import { defineProps, ref, watch } from 'vue';

const props = defineProps({
    list: String,
    memory: Memory
});

let addrToSym = ref({});
let values = ref({});


function update() {
    if (props.memory) {
        for (const addr of Object.keys(addrToSym.value)) {
            values.value[addr] = props.memory.getWord(addr);
        }
        requestAnimationFrame(update);
    }
}
requestAnimationFrame(update);


watch(() => props.list, (listing) => {
    addrToSym.value = {};
    if (listing) {
        const re = /^[0-9a-fA-F]{4}/;
        const xas99 = listing.startsWith("XAS99");
        const gcc = listing.includes("elf32-tms9900");
        for (let line of listing.split(/\r?\n/)) {
            if (line.includes(".... >") && line.trim().endsWith(" :")) {
                line = line.replaceAll(".", " ");
                line = line.split(/\s+/);
                line[2] = line[2].substr(1);
                const addr = parseInt(line[2], 16);
                const sym = line[1].toUpperCase();
                if (addr < 0x1000)
                    continue;
                if ( addr >= 0xF000)
                    continue;
                addrToSym.value[addr] = sym;
            }
        }
    }
}, { immediate: true })


</script>
<style scoped>
dl {
    display: grid;
    grid-template-columns: max-content auto;
}

dt {
    font-weight: bold;
    grid-column-start: 1;
    margin-bottom: 5px;
    font-family: monospace;
}

dd {
    grid-column-start: 2;
    margin-bottom: 5px;
}
</style>