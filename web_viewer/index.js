let cnv = document.getElementById("canvas");
let ctx = cnv.getContext("2d");
ctx.fillStyle = "#000000";
ctx.fillRect(0, 0, 1000, 1000);

function readSingleFile(e) {
  let file = e.target.files[0];
  if (!file) {
    return;
  }
  let reader = new FileReader();
  reader.onload = function(e) {

    ctx.fillStyle = "#000000";
    ctx.fillRect(0, 0, 1000, 1000);

    /**
     * @type ArrayBuffer
     */
    let contents = e.target.result;
    let img1Bytes = new Int8Array(contents.slice(0, 0x87b8));

    let charSize = 16
    let charArea = charSize*charSize
    let perRow = 10

    for(let i = 0; i < img1Bytes.length/2; i++ ) {

      let charno = Math.floor(i/charArea)
      let rowno = Math.floor(charno/perRow)

      let x = (i % charSize) + (charSize*charno) - (charSize*perRow*rowno)
      let y = Math.floor(i/charSize) - (charSize*charno) + (charSize*rowno)

      let rgba = (img1Bytes[i*2] << 8) | (img1Bytes[i*2 + 1])

      let r5 = (rgba & 0xF800) >> 11 //1111 1000 0000 0000
      let g5 = (rgba & 0x07C0) >> 6  //0000 0111 1100 0000
      let b5 = (rgba & 0x003E) >> 1  //0000 0000 0011 1110
      let a1 = (rgba & 0x0001)       //0000 0000 0000 0001

      let r = (r5/31)*255
      let g = (g5/31)*255
      let b = (b5/31)*255

      ctx.fillStyle = `rgba(${r}, ${g}, ${b}, ${a1})`//"rgba("+r+","+g+","+b+","+(a1)+")";
      ctx.fillRect(x*4, y*4, 4, 4);
    }
  };
  reader.readAsArrayBuffer(file)
}

document.getElementById("input")
  .addEventListener("change", readSingleFile, false);