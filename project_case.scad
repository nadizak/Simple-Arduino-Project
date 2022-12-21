
wall = 2;

base_x = 119.7;
base_y = 90.4;
case_x = base_x+2*wall;
case_y = base_y+2*wall;
case_z = 40;

lcd_x = 71.5;
lcd_z = 26.6;

module bread_board_aprx() {
    cube([53.4, 83.2, 9.3]);
}

module arduino_aprx() {
    cube([53.8, 72.1, 10]);
}

module case_bottom() {
  union() {
      // Bread board and arduino display
      % translate([case_x-2*wall-54, 2*wall,2*wall]) bread_board_aprx();
      % translate([2*wall, (case_y - 73)/2, 2*wall]) arduino_aprx();
      // Base
      cube([case_x, case_y, wall]);
      // Side walls
      cube([wall, case_y, case_z]);
      translate([case_x, 0, 0]) cube([wall, case_y, case_z]);
      // Front wall
      difference() {
          #cube([case_x, wall, case_z]);
          translate([case_x - wall*3 - lcd_x, -1, case_z-2*wall-lcd_z])
            cube([lcd_x, 6, lcd_z]);
      }
      
      //back wall
      difference() {
          #translate([0, base_y+wall, 0]) cube([case_x, wall, case_z]);
      }
  }
};

case_bottom();