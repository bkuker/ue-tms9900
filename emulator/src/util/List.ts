export class List {

    private list = [];

    constructor( listing : string ){
        const re = /^[0-9a-fA-F]{4}/;
        for (let line of listing.split(/\r?\n/)) {
            if ( re.test(line) ){
                let addr = parseInt(line.substring(0,4),16);
                let asm = line.substring(17);
                this.list[addr] = asm;
            }
        }
    }

    getLine( addr: number ): string {
        return this.list[addr];
    }
}