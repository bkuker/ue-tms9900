/**
 * The devices on the bus have open collectors on IC0-3 wire anding them together.
 * Each emulated device registers as an InterruptSource, providing it's own IC0-3
 * values and IntReq value. This code combines them together and presents a single
 * value to the CPU.
 * 
 * I think this could cause oddness if multiple interrupts occur at the same time
 * but this matches the current hardware
 * 
 */

export interface InterruptSource {
    getInterruptCode(): number;  // Returns 0-15, or 15 if not asserting
    isAssertingInterrupt(): boolean;
}

export class InterruptEncoder {
    private devices: InterruptSource[] = [];
    
    constructor() {

    }

    registerInterruptSource(device: InterruptSource) {
        this.devices.push(device);
        //Sort devices in reverse
        this.devices.sort((a, b) => a.getInterruptCode() - b.getInterruptCode());
    }

    getInterruptState(): number | false {
        //List is sorted descending, so return the highest
        for ( const d of this.devices ){
            if ( d.isAssertingInterrupt() )
                return d.getInterruptCode();
        }
        return false;
    }

}