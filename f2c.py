from os import write


def main():
    total = 0;
    w = open('font.c','w+')
    with open('font.txt') as f:
        c =f.readline()
        w.write("{")
        while(c):
            bits = f.readline()
            total+=1
            w.write("{")
            l = bits.split(',')
            l.pop()
            for byte in l:
                w.write("{")

                b = int(byte,16)
                for i in range(0,8):
                    mask = 1 << (7-i);
                    r = (b & mask) >> (7-i);
                    w.write(str(r));
                    if i<=6:
                        w.write(',')


                w.write("}")
                w.write(",")
                w.write('\n')
            w.write('},')
            w.write('\n')
            f.readline()
            c =f.readline()
        
        l = [] 
        for i in range(0,16):
            l.append("0x18")
        w.write("{")
        for byte in l:
            w.write("{")

            b = int(byte,16)
            for i in range(0,8):
                mask = 1 << (7-i);
                r = (b & mask) >> (7-i);
                w.write(str(r));
                if i<=6:
                    w.write(',')


            w.write("}")
            w.write(",")
            w.write('\n')
        w.write('}')
        w.write('\n')
            
        w.write("}")
    print(total)
    w.close()

if __name__ == "__main__":
    main()