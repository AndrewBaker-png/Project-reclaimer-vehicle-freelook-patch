// Inspect native PE import tables without loading either binary.
const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../build-freelook-mp-r30');
for(const name of ['Halo3Freelook.exe','Halo3VehicleFreelook-only-mp-r30.dll']){
 const b=fs.readFileSync(path.join(root,name)),pe=b.readUInt32LE(0x3c),coff=pe+4,opt=coff+20;
 if(b.toString('ascii',pe,pe+4)!=='PE\0\0'||b.readUInt16LE(opt)!==0x20b)throw Error('Expected x64 PE: '+name);
 const sectionStart=opt+b.readUInt16LE(coff+16),sections=b.readUInt16LE(coff+2);
 function offset(rva){for(let i=0;i<sections;i++){const s=sectionStart+i*40,va=b.readUInt32LE(s+12),size=Math.max(b.readUInt32LE(s+8),b.readUInt32LE(s+16));if(rva>=va&&rva<va+size)return b.readUInt32LE(s+20)+rva-va;}throw Error('Unmapped PE RVA');}
 const imports=[];let cursor=offset(b.readUInt32LE(opt+112+8));
 for(;b.readUInt32LE(cursor+12);cursor+=20){const start=offset(b.readUInt32LE(cursor+12)),end=b.indexOf(0,start);imports.push(b.toString('ascii',start,end));}
 if(imports.some(x=>/d3d|dxgi|dcomp|dwmapi|gdi32|opengl|vulkan|ws2_32|winhttp|wininet/i.test(x)))throw Error('Unexpected graphics/network dependency: '+imports);
 if(b.includes(Buffer.from('Halo3VisorHud','utf16le'))||b.includes(Buffer.from('cockpit-detail.bin','utf16le')))throw Error('Unexpected cockpit/HUD dependency');
 console.log(name+': '+imports.join(', '));
}
console.log('PASS: no graphics/network library imports or cockpit/HUD file dependencies');
