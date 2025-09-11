# mean filter

Just a quick implementation of a 2D mean filtering algorithm. 

I got tripped up by this in an interview and so wanted to come back and solve it.
Implementing the `mean_filter` function took me a little over an hour.
It's not perfect, I think it's more complicated than it needs to be and it's prone 
to underflowing pixel values when the kernel is small, but it does work I'll come back
and fix it in not too long.

The ethically sourced Lena image came from https://mortenhannemose.github.io/lena/
