import { treeparseHeaders } from "./tree.js";
if (global.gc) {
  global.gc();
} else {
  console.log('Garbage collection unavailable. Pass --expose-gc '
  + 'when launching node to enable forced garbage collection.');
}
await new Promise((resolve) => setTimeout(resolve, 500));

let rss = process.memoryUsage().rss

console.log(`usage ${Math.round(rss/ 1024 * 100) / 100} kB`);