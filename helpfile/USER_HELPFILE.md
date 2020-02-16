#### Alpha Blend Attributes
There's unfortunately no great ways to describe alpha blending
we could be hand-wavey and say it's a transparency value
but alpha blending and transparency are not the same thing
transparency color calculations are a subset of alpha blending
so if you have transparency, you will have a top color with some opacity level EVA, and a bottom color which is EVB = 1 - EVA
so green with 30% opacity on top of red would be some color that is 0.3 * red + 0.7 * green
however, with alpha blending, the relation does not hold true, EVB = 1 - EVA
it means that colors can additively go over 255 (they cap at 255)
so something like 0.7 * yellow + 0.6 * green will give you 1.3 times the green channel, making any pixels with 196 or more green max out at 255
but anyway, we call it the alpha blending attribute because WL4 has a few sets of EVA/EVB pairs that you can pick from (you can't set them individually)
