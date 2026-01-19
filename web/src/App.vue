<script setup lang="ts">
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { Memory } from '@ue-tms9900/emulator/memory';

    let memory = new Memory("");
    let cpu = new TMS9900(memory);
    cpu.reset();
</script>

<template>
  Hello
</template>

<style scoped>

</style>
