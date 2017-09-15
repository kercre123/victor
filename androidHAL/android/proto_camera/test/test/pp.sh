for f in *.gray
do
 echo "Processing $f"
 convert -size 640x360 -depth 8 $f  ${f}.png    
 # && open ${f}.png
done
convert  -loop 0 *.png animation.gif
rm *.png *.gray
open animation.gif
