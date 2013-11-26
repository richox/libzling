zling
=====

**fast and lightweight compression utility.**

zling (zlite-ng) is an improved compression utility, based on [zlite](https://github.com/richox/zlite).

zling shares the same entropy encoder with zlite, but uses an order-1 ROLZ, instead of zlite's order-3 one. and output index is encoded independently with another Polar table plus some extra bits.

in practice, zling compresses better and a bit slower than zlite, but decompresses faster.

simple benchmark with __enwik8__(100,000,000 bytes), with clang-3.2 (linux, -O3):

CPU: Intel Xeon E5-2620

MEM: 128GB

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>32291792</td>        <td>3.99s</td>  <td>1.05s</td></tr>
 <tr><td>gzip</td>  <td>36518322</td>        <td>8.13s</td>  <td>1.47s</td></tr>
</table>

simple benchmark with __fp.log__(20,617,071 bytes), with clang-3.2 (linux, -O3):

CPU: Intel Xeon E5-2620

MEM: 128GB

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>895961</td>          <td>0.14s</td> <td>0.07s</td></tr>
 <tr><td>gzip</td>  <td>1448582</td>         <td>0.41s</td> <td>0.14s</td></tr>
</table>
