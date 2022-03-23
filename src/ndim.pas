program ndim;

var arr1, arr2, result: array [0..1, 0..1] of integer;

procedure read2d(var arr: array [0..1, 0..1] of integer);
var i,j:integer;
begin
  for i := 0 to 1 do
	for j := 0 to 1 do
		read(arr[i, j])
end;

procedure write2d(var arr: array [0..1, 0..1] of integer);
var i,j:integer;
begin
  for i := 0 to 1 do
	for j := 0 to 1 do
		write(arr[i, j])
end;

procedure add2d(var left, right, result: array [0..1, 0..1] of integer);
var i,j:integer;
begin
  for i := 0 to 1 do
	for j := 0 to 1 do
		result[i, j] := left[i, j] + right[i, j]
end;
begin
  read2d(arr1);
  read2d(arr2);

  write2d(arr1);
  write2d(arr2);

  add2d(arr1, arr2, result);
  write2d(result)
end.