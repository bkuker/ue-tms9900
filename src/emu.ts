import { TMS9900 } from './emulator/classes/tms9900';
import { Memory } from './emulator/classes/memory';



main().catch((err) => {
    console.error(err);
    process.exit(1);
});



async function main(): Promise<void> {
    let memory = new Memory('Assembly/Programs/typewriter_v5.rom');
    let cpu = new TMS9900(memory);
    cpu.reset();

    while (true) {
        cpu.run(1000, false);
        await new Promise(r => setImmediate(r));
    }
}