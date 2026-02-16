<template>
    <div>
        <h2>History:</h2>
        <div v-for="h in cpu.history">
            pc: 0x{{ h.pc.toString(16).padStart(4, '0') }}
            sp: 0x{{ h.sp.toString(16).padStart(4, '0') }}<br>
            <span v-for="s in h.stack">{{ s.toString(16) }}, </span>
            <br> <br>
        </div>
    </div>
</template>
<script setup>
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990';
import { defineProps, ref, watch } from 'vue';

const props = defineProps({
    cpu: TMS9900
});

</script>
<style scoped>
</style>