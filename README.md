zling
=====

**fast and lightweight compression utility.**

zling (zlite-ng) is an improved compression utility, based on [zlite](https://github.com/richox/zlite).

zling shares the same entropy encoder with zlite, but uses an order-1 ROLZ, instead of zlite's order-3 one. and output index is encoded independently with another Polar table plus some extra bits.

in practice, zling compresses better and a bit slower than zlite, but decompresses faster.

simple benchmark with __enwik8__(100,000,000 bytes), with mingw-gcc-4.7.3, test was done on ramdisk:

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>32799061</td>        <td>3.557s</td> <td>1.186s</td></tr>
 <tr><td>gzip</td>  <td>36518322</td>        <td>6.635s</td> <td>1.268s</td></tr>
</table>

simple benchmark with __fp.log__(20,617,071 bytes), with mingw-gcc-4.7.3, test was done on ramdisk:

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>932887</td>          <td>0.156s</td> <td>0.094s</td></tr>
 <tr><td>gzip</td>  <td>1448582</td>         <td>0.392s</td> <td>0.122s</td></tr>
</table>
