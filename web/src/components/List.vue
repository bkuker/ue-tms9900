<template>
    <div class="listing">
        <span v-for="(line) in lines" :class="{ current: pc == line.addr, addr: line.addr }"
            :style="{ color: 'hsl(0, ' + (100 * Math.pow(profile[line.addr] / max, .4)) + '%, 50%)' }"
            @dblclick="$emit('runTo', line.addr)">
            {{ line.addr?line.addr.toString(16).padStart(4,'0')+': ':"      " }} {{ line.text + "\n" }}
        </span>
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

let lines = ref([]);

let pc = ref();
let profile = ref([]);
let max = ref(0);


function update() {
    let cpu = props.cpu;
    if (cpu) {
        pc.value = cpu.getPc();
        profile.value = cpu.getProfile();
        let m = 0;
        for (let line of lines.value) {
            if (line.addr)
                m = Math.max(m, profile.value[line.addr]);
        }
        max.value = m;
    }
    requestAnimationFrame(update);
}
requestAnimationFrame(update);

watch(() => props.list, (listing) => {
    lines.value = [];
    if (listing) {

        const xas99 = listing.startsWith("XAS99");
        const gcc = listing.includes("elf32-tms9900");

        if (listing.includes("elf32-tms9900")) {
            const re = /^[0-9a-fA-F]{1,4}:/;
            for (let line of listing.split(/\r?\n/)) {
                let l = {};
                l.text = line;
                if (re.test(line.trim())) {
                    //is an address
                    let addr = line.trim();
                    addr = addr.substring(0, addr.indexOf(':'));
                    addr = parseInt(addr, 16);
                    l.addr = addr;
                }
                if (l.text.trim().length > 0)
                    lines.value.push(l);
            }
        } else if (listing.startsWith("XAS99")) {
            const re = /^[0-9a-fA-F]{4}/;
            for (let line of listing.split(/\r?\n/)) {
                let l = {};
                l.text = line = line.substring(5);
                if (re.test(line)) {
                    let addr = parseInt(line.substring(0, 4), 16);
                    let asm = line;
                    if (asm) {
                        l.addr = addr;
                    }
                }
                l.text = line = line.substring(14);
                if (l.text.trim().length > 0)
                    lines.value.push(l);
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
    background-color: #252525;
    color: #d4d4d4;
    padding: 20px;
}

.listing.columns {
    column-width: 440px;
    column-gap: 10px;
}

.listing>span {
    display: block;
    width: 450px;
    overflow: hidden;
}

.listing>span.addr {
    cursor: pointer;
}

.listing>span:hover.addr {
    background-color: rgb(99, 99, 99);
}

.current {
    background-color: #193d26
}
</style>