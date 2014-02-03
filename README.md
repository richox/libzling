zling
=====

*fast and lightweight compression utility and library.*

zling is an improved lightweight compression utility and library.

zling uses fast order-1 ROLZ (16MB block size and 10MB dictionary size) followed with Huffman encoding, making it twice as fast as gzip on compressing, while still getting better compression ratio and decompression speed.

simple benchmark with _enwik8_(100,000,000 bytes)

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>32196557</td>        <td>3.18s</td>  <td>1.02s</td></tr>
 <tr><td>gzip</td>  <td>36518322</td>        <td>8.13s</td>  <td>1.47s</td></tr>
</table>

simple benchmark with _fp.log_(20,617,071 bytes)

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>897456</td>          <td>0.13s</td> <td>0.07s</td></tr>
 <tr><td>gzip</td>  <td>1448582</td>         <td>0.41s</td> <td>0.14s</td></tr>
</table>
