import { TMS9900 } from './emulator/classes/tms9900';
import { Memory } from './emulator/classes/memory';
import { CRU } from './emulator/classes/cru';



main().catch((err) => {
    console.error(err);
    process.exit(1);
});



async function main(): Promise<void> {
    let memory = new Memory('Assembly/Programs/hellorld.rom');
    let cru = new CRU();
    let cpu = new TMS9900(memory, cru);
    cpu.reset();

    while (true) {
        cpu.run(1000, false);
        await new Promise(r => setImmediate(r));
    }
}