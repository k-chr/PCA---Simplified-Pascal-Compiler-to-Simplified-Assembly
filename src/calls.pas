program calls;
var x, y: integer;

procedure callbyref(var x: integer);
begin
  x := 2137
end;

procedure callbyval(x: integer);
begin
  x := 7312
end;

begin
  callbyref(x);
  callbyval(y);
  write(x, y)
end.