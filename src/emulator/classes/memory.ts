
import fs from 'fs';

import { Log } from '../../classes/log';
import { CPU } from '../interfaces/cpu';
import { UeUart } from './ueuart';

export class Memory {


    private log: Log = Log.getLog();
    private rom: Uint8Array;
    private ram: Uint8Array = new Uint8Array(12 * 1024);

    private mux1 = new UeUart(0xF000);

    constructor(rom: string) {
        console.log(`Loading ${rom} @0x0000`);
        const buffer = fs.readFileSync(rom);
        this.rom = new Uint8Array(buffer);
    }

    public reset(keepCart: boolean) {

    }


    getWord(addr: number): number {
        return this.readWord(addr, null);
    };


    public readWord(addr: number, cpu: CPU | null): number {
        //TODO ADD CYCLES

        var ret: number;
        if (addr >= 0 && addr < 0x0FFE) {
            //ROM
            ret = (this.rom[addr] << 8) | this.rom[addr + 1];
        } else if (addr >= 0x1000 && addr < 0x3FFE) {
            cpu?.addCycles(4);
            const offset = addr - 0x1000;
            ret = (this.ram[offset] << 8) | this.ram[offset + 1];
            //RAM
        } else if (addr >= 0xF000 && addr <= 0xF008) {
            ret = this.mux1.readWord(addr);
        } else {
            this.log.info(`Read from unmapped location ${addr.toString(16)}`);
            ret = 0;
        }
        //console.log(`readWord ${addr.toString(16)}: ${ret}`);
        return ret;
    }

    public writeWord(addr: number, w: number, cpu: CPU) {
        //if (w)
            //console.log(`writeWord ${addr.toString(16)}: ${w.toString(16)}`);

        if (addr >= 0 && addr < 0x0FFE) {
            this.log.info(`Write to ROM location ${addr.toString(16)}`);
        } else if (addr >= 0x1000 && addr < 0x3FFE) {
            cpu.addCycles(4); //Probably a TI99 thing, ram behind video processor
            const offset = addr - 0x1000;
            this.ram[offset] = w >> 8;
            this.ram[offset + 1] = w & 0xFF;
        } else if (addr >= 0xF000 && addr <= 0xF008) {
            this.mux1.writeWord(addr, w);
        } else {
            this.log.info(`Write to unmapped location ${addr.toString(16)}`);
        }



    }

}
