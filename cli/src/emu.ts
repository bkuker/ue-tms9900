import test from '@ue-tms9900/emulator/test';

import { TMS9900 } from '@ue-tms9900/emulator/emulator/classes/tms9900';
import { Memory } from '@ue-tms9900/emulator/emulator/classes/memory';
import { Command } from 'commander';

//Get Command Line
const commandLine = new Command();
commandLine
    .argument('<rom file name>');
commandLine.parse();

main().catch((err) => {
    console.error(err);
    process.exit(1);
});


async function main(): Promise<void> {
    let memory = new Memory(commandLine.args[0]);
    let cpu = new TMS9900(memory);
    cpu.reset();

    //Run the CPU Continuously...
    while (true) {
        cpu.run(1000, false);
        //But yeild every so often so UARTs and other things 
        //do their thing
        await new Promise(r => setImmediate(r));
    }
}