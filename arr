#!/usr/bin/env ruby
# arr EXPR [FILES...] - (re)arrange and select fields on each line
#
# To the extent possible under law,
# Christian Neukirchen <chneukirchen@gmail.com>
# has waived all copyright and related or neighboring rights to this work.
# http://creativecommons.org/publicdomain/zero/1.0/

USAGE = <<'EOF'
Usage: arr [-0] [-P|-p PADDING] EXPR [FILES...]
  EXPR   ::= FIELDS (("|" CHAR | "*") FIELDS)* ("&" CHAR)?
                     # | split on char, * split bytes, & join with char
  FIELDS ::= "~"? FIELD ("," FIELD)*          # ~ negates
  FIELD  ::= "-"? "\d"+                       # negative fields count from back
           | ("-"? "\d"+)? ":" ("-"? "\d"+)?  # range ends default to 1:-1
EOF

require 'strscan'
require 'optparse'

def fmt(ss, d)
  last_split = " "

  loop {
    fields = []
    neg = false

    begin
      i = j = nil
      neg = true  if ss.scan(/~/)
      if ss.scan(/:|-?\d+/)
        i = Integer(ss.matched) rescue 1
        i = i < 0 ? d.size + i : i - 1
        if ss.matched == ":" || ss.scan(/:/)
          if ss.scan(/-?\d+/)
            j = Integer(ss.matched)
          else
            j = -1
          end
          j = j < 0 ? d.size + j : j - 1

          if j > i
            fields.concat (i..j).to_a
          else
            fields.concat (j..i).to_a.reverse
          end
        else
          fields << i
        end
      else
        abort "parse error at #{ss.inspect}"
      end
    end while ss.scan(/,/)
    
    d = d.values_at(*if neg
                       (0..d.size).to_a - fields
                     else
                       fields
                     end)
        
    if ss.scan(/\|(.)/)
      d = d.join(last_split)
      last_split = ss[1]
      d = d.split(last_split)
    elsif ss.scan(/\*/)
      d = d.join(last_split)
      last_split = ""
      d = d.split('')
    else
      break
    end
  }

  if ss.scan(/\&(.)/)
    last_split = ss[1]
  end

  unless ss.scan(/\}/)
    abort "parse error at #{ss.inspect}"
  end
    
  d.compact.join(last_split)
end

def fmt2(str, arr)
  ss = StringScanner.new(str)
  
  r = ""

  while ss.scan(/(.*?)%\{/)
    r << ss[1]
    r << fmt(ss, arr)
  end

  r << ss.rest
end

begin
  params = ARGV.getopts('0Pp:')
  nl = params["0"] ? "\0" : "\n"
  padding = params["p"] || ""
  padding = nil  if params["P"]
  
  expr = ARGV.shift  or raise OptionParser::MissingArgument, "no EXPR given"

  ARGV << "-"  if ARGV.empty?

  files = ARGV.map { |name|
    if name == "-"
      STDIN
    else
      File.open(name, "rb")
    end
  }

  until files.all?(&:eof?)
    lines = files.map { |f| f.gets(nl).chomp(nl) rescue padding }
    break  if lines.include?(nil)
    print fmt2(expr, lines), nl
  end
rescue OptionParser::ParseError
  STDERR.puts $!
  STDERR.puts USAGE
end
