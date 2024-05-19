import array
from pathlib import Path
import wave


def wav2byt(base):
    byte_array = array.array('B')

    wavefile = wave.open(path + "\\" + base + ".wav", 'rb')
    (nchannels, samplewidth, framerate, nframes, comptype, compname) = wavefile.getparams()

    print("channels:", nchannels)
    print("sample width:", samplewidth)
    print("framerate:", framerate)
    print("no of frames:", nframes)
    print("comptype:", comptype)
    print("compname:", compname)

    wavedata = wavefile.readframes(nframes)

    wavefile.close()

    byte_array.frombytes(wavedata)
    print(len(byte_array))

    #  file_out_b = open(path+"\\"+base+".byt", 'wb')
    #  file_out_b.write(byte_array)
    #  file_out_b.close()

    file_out_c.write("//File %s\n" % base)
    file_out_c.write("//framerate %i\n" % framerate)
    file_out_c.write("//no of frames %i\n" % nframes)

    file_out_c.write("const uint8_t " + base + "[] = {\n")
    c = 0
    for b in byte_array:
        file_out_c.write("0x%02x" % b)
        file_out_c.write(",")
        c = c + 1
        if (c > 20):
            file_out_c.write("\n")
            c = 0
    file_out_c.write("0};\n//-------------------------------------------------------------------\n")
    return len(byte_array)


path = "X:\pi-telecow"

file_out_c = open(path + "\\nokiaring.c", 'w')

file_out_c.write("//Created by wav2byt.py\n")
file_out_c.write("//---------------------------------------------------------------\n")
file_out_c.write("\n")

file = path + "\\" + "nokia6210-02-ring-ring.wav"

base = Path(file).stem
cc = wav2byt(base)
file_out_c.write("// length:"+str(cc))
file_out_c.write("\n")


