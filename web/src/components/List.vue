<template>
    <div class="listing">
        <span v-for="(line, addr) in list" :class="{ current: pc == addr }"
            :style="{ color: 'hsl(0, ' + (100 * Math.pow(profile[addr] / max, .4)) + '%, 50%)' }">{{ line + "\n" }}</span>
    </div>
</template>
<script setup>
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990';
import { defineProps, ref, watch } from 'vue';

const props = defineProps({
    list: String,
    cpu: TMS9900
});

let list = ref({});
let pc = ref();
let profile = ref([]);
let max = ref(0);

function update() {
    let cpu = props.cpu;
    if (cpu) {
        pc.value = cpu.getPc();
        profile.value = cpu.getProfile();
        let m = 0;
        for (let addr of Object.keys(list.value)) {
            m = Math.max(m, profile.value[addr]);
        }
        max.value = m;
    }
    requestAnimationFrame(update);
}
requestAnimationFrame(update);

watch(() => props.list, (listing) => {
    list.value = {};
    if (listing) {
        const re = /^[0-9a-fA-F]{4}/;
        const xas99 = listing.startsWith("XAS99");
        const gcc = listing.includes("elf32-tms9900");
        for (let line of listing.split(/\r?\n/)) {
            if (gcc && line[4] == ':') {
                let addr = parseInt(line.substring(0,4).trim(),16);
                list.value[addr] = line;
            } else {
                if (xas99)
                    line = line.substring(5);
                if (re.test(line)) {
                    let addr = parseInt(line.substring(0, 4), 16);
                    let asm = line;//.substring(xas99?14:17);
                    if (asm)
                        list.value[addr] = asm;
                }
            }
        }
    }
}, { immediate: true })


</script>
<style scoped>
.listing {
    white-space: pre;
    font-family: monospace;
    font-size: smaller;
    column-width: 440px;
    column-gap: 10px;
    background-color: #252525;
    color: #d4d4d4;
    padding: 20px;
}

.listing>span {
    display: block;
    width: 450px;
    overflow: hidden;
}

.current {
    background-color: #2e7548
}
</style>