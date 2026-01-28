<template>
    <div class="listing">
        <span v-for="(line, addr) in list" :class="{ current: pc == addr }" :style="{ color: 'hsl(0, '+(100*Math.pow(profile[addr]/max,.4))+'%, 50%)' }">{{ line + "\n" }}</span>
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
        for ( let addr of Object.keys(list.value)){
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
        for (let line of listing.split(/\r?\n/)) {
            if ( xas99 )
                line = line.substring(5);
            if (re.test(line)) {
                let addr = parseInt(line.substring(0, 4), 16);
                let asm = line.substring(xas99?14:17);
                if ( asm )
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
    column-width: 450px;
    column-gap: 20px;
    background-color: #252525;
    color: #d4d4d4;
    padding: 20px;
}

.current {
    background-color: #2e7548
}
</style>