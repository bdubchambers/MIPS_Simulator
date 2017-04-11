#TCSS 372 Comp.Arch.
#MIPS Simulator/Assembler: Test file
#Brandon Chambers
#Thomas Xu
#
#comments work on their own line, not extra blank lines or endofline comments
addi $s0, $s0, 20
addi $s1, $s1, 30
add $t3, $s1, $s0
#s0 contains 20, add one word=4 to it, and we have
#the value '21' to be used as an address to store the 
#value of reg. $t3 (50) at....so RAM[21] == 50
sw $t3, 4($s0)
#store the number 21 in a new register, $a0, to be 
#used as a pointer address for our 'lw' instr
addi $a0, $a0, 21
#now that we have a pointer addr stored in a reg., 
#we can use lw to load a value that is stored in memory
# into the specified register (in this case, $at)...
#the result is that register $at == 50, because our 
#memory RAM[21] == 50
lw  $at, 0($a0) 
sw  $s0, 4($a0)
sw  $s1, 8($a0)
sw  $t3, 12($a0)
sw  $a0, 16($a0)
lw  $fp, 12($a0)
and $ra, $fp, $at
addi $t5, $t5, 2
sub $ra, $ra, $t5
and $t5, $t5, $zero
addi $t6, $t6, 128
or $t7, $t6, $ra
addi $v0, $v0, 2
addi $v1, $v1, 3
mul  $t0, $v0, $v1
#program setup to require 'halt' instruction to end assembly code
halt
