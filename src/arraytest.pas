program arraytest(input,output);
var j,i,o:integer;
var p :array [1..10, 1..3] of integer;
var row :array [1..3] of integer;
var scalar: array [0..0] of integer;
var b:integer;

begin
  p[1, 1] := 3;
  row := p[1];
  scalar[0] := 1;
  write(p[1, 1])
end.
