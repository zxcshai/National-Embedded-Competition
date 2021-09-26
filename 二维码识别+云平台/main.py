import sensor, image, time
from pyb import UART
green_threshold = (0,100,   -128,0,   0,127)
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.VGA)
sensor.set_windowing((320, 320))
sensor.skip_frames(30)
sensor.set_auto_gain(True)

uart = UART(3, 9600)
def CodeArr(img):
    RightCode=0
    ColorCode=0
    for code in img.find_qrcodes():
        url=code[4].split('?')
        if url[0]=='https://h5.dingtalk.com/healthAct/index.html' or url[0]=='https://healthcode.dingtalk.com/healthAct/index.html':
            RightCode=1;
            print("扫描到二维码")
        else:
            RightCode=2;
        if RightCode==1:
            img.binary([green_threshold])
            CodeX=code[0]
            CodeY=code[1]
            CodeW=code[2]
            CodeH=code[3]
           # print("x:{},y:{},w:{},h:{}",CodeX,CodeY,CodeW,CodeH)
            square=CodeW*CodeH/3
            count=0
            EndX=(CodeX+CodeW)/1.732
            EndY=(CodeY+CodeH)/1.732
            for X in range(CodeX,EndX):
                for Y in range(CodeY,EndY):
                    if img.get_pixel(X,Y)==(255,255,255): count=count++1
            print("count:",count)
            rate=count/square
            print("square:",rate)
            if rate>0.07 : ColorCode=1
            else : ColorCode=2
    return str(RightCode)+str(ColorCode)

while(True):
    img = sensor.snapshot()
    Code=CodeArr(img)
    if Code=='00':
       Data=0
    elif Code=='11':
       Data=1
    elif Code=='20':
       Data=2
    else :
       Data=3
    if Data!=0:
        output_str="%d" %(Data)
        uart.write(output_str+'\n')
        print("you send:",output_str)
        time.sleep_ms(500)
    time.sleep_ms(10)
