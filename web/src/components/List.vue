<template>
    <div class="listing">
        <span v-for="(line, addr) in list" :class="{ current: pc == addr }">{{ line + "\n" }}</span>
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

function update() {
    let cpu = props.cpu;
    if (cpu) {
        pc.value = cpu.getPc();
    }
    requestAnimationFrame(update);
}
requestAnimationFrame(update);

watch(() => props.list, (listing) => {
    console.log(listing);
    list.value = {};
    if (listing) {
        const re = /^[0-9a-fA-F]{4}/;
        for (let line of listing.split(/\r?\n/)) {
            if (re.test(line)) {
                let addr = parseInt(line.substring(0, 4), 16);
                let asm = line.substring(17);
                list.value[addr] = asm;
            }
        }
    }
}, { immediate: true })


</script>
<style scoped>
.listing {
    white-space: pre;
    font-family: monospace;
    font-size: small;
    column-width: 430px;
    column-gap: 20px;
    background-color: #252525;
    color: #d4d4d4;
    padding: 20px;
}

.current {
    background-color: #2e7548
}
</style>