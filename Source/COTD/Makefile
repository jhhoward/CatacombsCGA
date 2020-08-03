obj = Main.obj Draw.obj Enemy.obj Entity.obj FixedMath.obj Game.obj MapGenerator.obj Map.obj Menu.obj Particle.obj Player.obj Projectile.obj Sounds.obj DOSLib.obj
bin = COTD.exe

CC = wpp
CFLAGS = -zq -0 -ot -bt=DOS -w2 -mm
LD = wlink

$(bin): $(obj)
    $(LD) system dos name $@ file { $(obj) }

.cpp.obj:
    $(CC) -fo=$@ $(CFLAGS) $<

clean: .symbolic
    del *.obj
    del $(bin)