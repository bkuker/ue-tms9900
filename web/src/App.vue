<script setup lang="ts">
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { Memory } from '@ue-tms9900/emulator/memory';

async function start() {
  const response = await fetch("hellorld.rom");
  const arrayBuffer = await response.arrayBuffer();
  const romContents = new Uint8Array(arrayBuffer);


  let memory = new Memory(romContents);
  let cpu = new TMS9900(memory);
  cpu.reset();

    while (true) {
        cpu.run(1000, false);
        //But yeild every so often so UARTs and other things 
        //do their thing
        await new Promise(r => setTimeout(r, 0));
    }
}

start();
</script>

<template>
  Hello
</template>

<style scoped></style>
