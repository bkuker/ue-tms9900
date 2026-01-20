


import { Log } from './util/log';
import { CPU } from './interfaces/cpu';
import { UeUart } from './ueuart';

const log: Log = Log.getLog();

export interface MemoryMapped {
    getBaseAddress(): number;
    getSize(): number;
    readWord(addr: number, cpu: CPU | null): number;
    writeWord(addr: number, w: number, cpu: CPU);
}

class RAM implements MemoryMapped {
    private size;
    private baseAddr: number;
    protected ram: Uint8Array;

    constructor(baseAddr: number, size: number) {
        this.baseAddr = baseAddr;
        this.ram = new Uint8Array(size);
        this.size = size;
    }

    readWord(addr: number, cpu: CPU | null): number {
        cpu?.addCycles(4);
        const offset = addr - this.baseAddr;
        return (this.ram[offset] << 8) | this.ram[offset + 1];
    }

    writeWord(addr: number, w: number, cpu: CPU) {
        cpu.addCycles(4); //Probably a TI99 thing, ram behind video processor
        const offset = addr - 0x1000;
        this.ram[offset] = w >> 8;
        this.ram[offset + 1] = w & 0xFF;
    }

    public getBaseAddress() {
        return this.baseAddr;
    };

    public getSize() {
        return this.size;
    }
}

class ROM extends RAM {
    constructor(baseAddr: number, size: number, romData: Uint8Array) {
        super(baseAddr, size);
        this.ram.set(romData);
    }

    writeWord(addr: number, w: number, cpu: CPU) {
        log.warn(`Write to ROM @0x${addr.toString(16)}`);
    }
}

export class Memory {
    private log: Log = Log.getLog();
    private rom: Uint8Array = new Uint8Array(12 * 1024); //TODO
    private ram: Uint8Array = new Uint8Array(12 * 1024);

    mux0: UeUart;
    mux1: UeUart;

    private map: MemoryMapped[];

    constructor(romData: Uint8Array) {
        this.map = [
            new ROM(0x0000, 2 * 1024, romData),
            new RAM(0x1000, 12 * 1024),
            this.mux0 = new UeUart(0xF000),
            this.mux1 = new UeUart(0XF00A)
        ];
    }

    getWord(addr: number): number {
        return this.readWord(addr, null);
    };


    public readWord(addr: number, cpu: CPU | null): number {
        addr &= 0xFFFE;
        //TODO ADD CYCLES

        for (const mm of this.map) {
            //console.log(mm.getBaseAddress(), mm.getSize(), addr);
            if (addr >= mm.getBaseAddress() && addr < mm.getBaseAddress() + mm.getSize()) {
                return mm.readWord(addr, cpu);
            }
        }
        this.log.info(`Read from unmapped location ${addr.toString(16)}`);
        return 0;
    }

    public writeWord(addr: number, w: number, cpu: CPU) {
        addr &= 0xFFFE;

        for (const mm of this.map) {
            if (addr >= mm.getBaseAddress() && addr < mm.getBaseAddress() + mm.getSize()) {
                return mm.writeWord(addr, w, cpu);
            }
        }
        this.log.info(`Write to unmapped location ${addr.toString(16)}`);
        return;
    }

}
