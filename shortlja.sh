# usage: ./shortlja [analysis type] [number of run] [read location]

run=$1
read_location=$2

read_name=$(echo "$read_location" | sed -r "s/.+\/(.+)\..+/\1/") # strips everything except filename
lja_output="lja-output/${read_name}/run${run}"

rm -rf "$lja_output/*" # clear directory in case of repeating run number
mkdir -p $lja_output
lja -o $lja_output --reads $read_location > "${lja_output}/stdout.txt" 2>  "${lja_output}/stderr.txt"

if [ -s "${lja_output}/stdout.txt" ]
then
   cat "${lja_output}/stdout.txt" | grep " time:"
else
   cat "${lja_output}/stderr.txt"
fi