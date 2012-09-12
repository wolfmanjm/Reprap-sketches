require 'csv'

if ARGV.size < 2
  puts "Usage: csv2xl.rb in out"
  exit 1
end

table=   Array.new(11) { Array.new(11) }
CSV.foreach(ARGV[0]) do |row|
  # use row here...
  #p row
  x= row[0].to_i/10
  y= row[1].to_i/10
  z= row[2].to_f
  table[y][x]= z
end

#p table

CSV.open(ARGV[1], "wb") do |csv|
  table.each {|r| csv << r }
end
