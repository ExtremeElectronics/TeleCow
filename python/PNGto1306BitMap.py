from PIL import Image, ImageOps
import math
basepath = "c:\\temp\\"
name = "telecow1306"
im = Image.open(basepath+name+".png") # Can be many different formats.
imgs = ImageOps.grayscale(im)
pix = imgs.load()

(xmax, ymax) = im.size   # Get the width and height of the image for iterating over
print(name, " X", xmax, " y", ymax)
out = open(basepath+name+".c", "w")
out.write("// From "+name+"\n")
out.write("//img_x_max="+str(xmax)+";\n")
out.write("//img_y_max="+str(ymax)+";\n")
size = xmax*math.ceil(ymax/8)+1
out.write("uint8_t "+name+"["+str(size)+"] = {\n    ")
for y in range(math.ceil(ymax/8)):
    for x in range(xmax):
        bits = 0
        for yb in range(8):
            value = pix[ x, y*8+yb]  # Get the RGBA Value of the a pixel of an image
            if value < 128:
                bits += 1 << yb
        out.write("0x%0.2X" % bits + ",")
    out.write("\n    ")
out.write("};\n")
out.close()

