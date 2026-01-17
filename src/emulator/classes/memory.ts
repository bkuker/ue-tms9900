
import fs from 'fs';

import { Log } from '../../classes/log';
import { CPU } from '../interfaces/cpu';

export class Memory {


    private log: Log = Log.getLog();
    private rom: Uint8Array;
    private ram: Uint8Array = new Uint8Array(12 * 1024);

    constructor() {
        const rom = 'Assembly/Programs/hellorld.rom';
        console.log(`Loading ${rom} @0x0000`);
        const buffer = fs.readFileSync(rom);
        this.rom = new Uint8Array(buffer);
    }

    public reset(keepCart: boolean) {

    }


    getWord(addr: number): number {
        return 0;
        //TODO This should get value with no cycles
        //return this.readWord(addr);
    };


    public readWord(addr: number, cpu: CPU): number {
        //TODO ADD CYCLES
        //console.log("Address Bus: " + addr.toString(2).padStart(16, "0").replaceAll("0", "."));
        if (addr >= 0 && addr < 0x0FFE) {
            //ROM
            return (this.rom[addr] << 8) | this.rom[addr + 1];
        } else if (addr >= 0x1000 && addr < 0x3FFE) {
            const offset = addr - 0x1000;
            return (this.ram[offset] << 8) | this.ram[offset + 1];
            //RAM
        } else {
            this.log.info(`Read from unmapped location ${addr.toString(16)}`);
            return 0;
        }
    }

    public writeWord(addr: number, w: number, cpu: CPU) {
        //console.log(`writeWord ${addr.toString(16)}: ${w.toString(16)}`);

        if (addr >= 0 && addr < 0x0FFE) {
            this.log.info(`Write to ROM location ${addr.toString(16)}`);
        } else if (addr >= 0x1000 && addr < 0x3FFE) {
            const offset = addr - 0x1000;
            this.ram[offset] = w >> 8;
            this.ram[offset + 1] = w & 0xFF;
        } else if ( addr == 0xF002 ){
            console.log("Mux1 Write " + String.fromCharCode(w));
        } else {
            this.log.info(`Write to unmapped location ${addr.toString(16)}`);
        }



    }

}
