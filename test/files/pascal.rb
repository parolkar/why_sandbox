def pascal n
  rows = []

  # generate data
  (0...n).each do |i|
    rows << if i.zero?
      [1]
    else
      rows[i-1].inject([0]) do |m, o|
        m[0...-1] << (m[-1] + o) << o
      end
    end
  end

  # calc field width
  width = rows[-1].max.to_s.length

  # space out each row
  rows.collect! do |row|
    row.collect { |x| x.to_s.center(2 * width) }.join
  end

  # display triangle
  rows.map { |row| row.center(rows[-1].length) }
end
