/*  Author: Ben Gibbs
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 */

#include <stdlib.h>
#include <stdio.h>

#include <avr/interrupt.h>

#include "ili934x.h"
#include "lcd.h"
#include "ruota.h"
#include "chess.h"



uint8_t get_piece_type_at(uint8_t, uint8_t, game_state);
uint8_t get_piece_first_state_at(uint8_t, uint8_t, game_state);
uint8_t get_piece_index_by_type(uint8_t, uint8_t, uint8_t, game_state);
int8_t get_piece_index_in_game_state(piece, game_state);

uint8_t calc_x_difference_from_past(uint8_t, uint8_t);
uint8_t calc_y_difference_from_past(uint8_t, uint8_t);

uint8_t can_move_pawn(uint8_t, game_state);
uint8_t can_move_rook(uint8_t, game_state);
uint8_t can_move_knight(uint8_t, game_state);
uint8_t can_move_bishop(uint8_t, game_state);
uint8_t detect_castling(uint8_t, game_state);
uint8_t can_move_king(uint8_t, game_state);

uint8_t is_move_valid(piece, game_state);
uint8_t check_in_check(game_state);
uint8_t check_checkmate(game_state);

uint8_t will_take_piece(game_state);
game_state take_piece(game_state);
rectangle get_piece_rectangle_from_coords(uint8_t, uint8_t, game_state);
game_state move_and_possibly_take_piece(game_state);

uint8_t is_turn_valid(piece p, game_state);

uint8_t is_possible_move_for_piece(piece, game_state);
uint8_t are_there_possible_moves(game_state);
move_set get_possible_moves_for_piece(piece, game_state);

uint8_t is_pawn_at_other_side();
uint8_t get_index_of_pawn_at_other_side();
void check_switches();

void change_turn();

rectangle get_rectangle_for_selector();
void update_selected();

void create_board();
void create_pieces();
void create_selector();
void first_draw();
void init_game();

void draw_possible_moves();
void draw_select(selector);
void draw_piece(piece);
void draw_tile(tile);

void refresh_tile(uint8_t, uint8_t, game_state);
void draw_over_potential_moves(move_set, game_state);
void refresh_en_passant_tile(game_state);
void refresh_castling_tile(game_state);

int main();






// First move is always white
// Team: 0 (White), 1 (BLACK)
// Type (WHITE): 1 (Pawn), 2 (Rook), 3 (Knight), 4 (Bishop), 5 (Queen), 6 (King)
// Type (BLACK): 7 (Pawn), 8 (Rook), 9 (Knight),10 (Bishop),11 (Queen),12 (King)





volatile game_state current_state;
volatile move_set current_move_set;
volatile tile board[64];








ISR(INT6_vect) {
    cli();
    
    // TODO USE

    sei();
}

ISR(TIMER1_COMPA_vect) {
    cli();

    scan_switches();

    sei();
}

ISR(TIMER3_COMPA_vect) {
    // TODO USE
}






uint8_t get_piece_type_at(uint8_t temp_x, uint8_t temp_y, game_state g) {
    uint8_t i;
    for (i=0; i<32; i++) {
        if (g.pieces[i].x==temp_x && g.pieces[i].y==temp_y && i!=g.selected_piece_index && g.pieces[i].taken==0) return g.pieces[i].type;
    }

    return 0;
}

uint8_t get_piece_first_state_at(uint8_t temp_x, uint8_t temp_y, game_state g) {
    uint8_t i;
    for (i=0; i<32; i++) {
        if (g.pieces[i].x==temp_x && g.pieces[i].y==temp_y && i!=g.selected_piece_index && g.pieces[i].taken==0) return g.pieces[i].first;
    }

    return 0;
}

uint8_t get_piece_index_by_type(uint8_t type, uint8_t x, uint8_t y, game_state g) {
    uint8_t i;
    for (i=0; i<32; i++) {
        if (g.pieces[i].x==x && g.pieces[i].y==y && g.pieces[i].taken==0 && g.pieces[i].type==type) return i;
    }

    return 0;
}

int8_t get_piece_index_in_game_state(piece p, game_state g) {
    uint8_t i;
    for (i=0; i<32; i++) {
        if (g.pieces[i].x==p.x && g.pieces[i].y==p.y && g.pieces[i].taken==0 && g.pieces[i].type==p.type) return i;
    }

    return 0;
}







uint8_t calc_x_difference_from_past(uint8_t x, uint8_t p_x) {
    if (x>p_x)
        return x-p_x;
    else
        return p_x-x;
}

uint8_t calc_y_difference_from_past(uint8_t y, uint8_t p_y) {
    if (y>p_y)
        return y-p_y;
    else
        return p_y-y;
}







uint8_t can_move_pawn(uint8_t index, game_state g) {
    piece p = g.pieces[index];
    uint8_t temp_type;

    if (p.type==1) {
        if (p.first==1) {
            if (p.y==(g.past_y-2)) {
                if (p.x==g.past_x) {
                    if (get_piece_type_at(p.x, p.y, g)==0) {
                        return 1; // FIRST MOVE so can move 2 forwards
                    }
                }
            }
        }


        if (p.y==(g.past_y-1)) {
            if (p.x==g.past_x)
                if (get_piece_type_at(p.x, p.y, g)==0) 
                    return 1; // IF STAYED IN SAME COLUMN then allow move if square empty

            if (p.x==(g.past_x-1) || p.x==(g.past_x+1)) {
                temp_type = get_piece_type_at(p.x, p.y, g);
                if (temp_type>6) {
                    return 1; // IF MOVED DIAGONALLY then allow move if square is occupied by opponents piece
                }
            }
        }

        if (g.can_en_passant) {
            if (p.y==g.en_passant_y-1 && p.x==g.en_passant_x && g.past_y==g.en_passant_y) {
                return 1;
            }
        }
    } else {
        if (p.first==1) {
            if (p.y==(g.past_y+2)) {
                if (p.x==g.past_x) {
                    if (get_piece_type_at(p.x, p.y, g)==0) {
                        return 1; // FIRST MOVE so can move 2 forwards
                    }
                }
            }
        }


        if (p.y==(g.past_y+1)) {
            if (p.x==g.past_x)
                if (get_piece_type_at(p.x, p.y, g)==0) 
                    return 1; // IF STAYED IN SAME COLUMN then allow move if square empty

            if (p.x==(g.past_x-1) || p.x==(g.past_x+1)) {
                temp_type = get_piece_type_at(p.x, p.y, g);
                if (temp_type<7 && temp_type>0) {
                    return 1; // IF MOVED DIAGONALLY then allow move if square is occupied by opponents piece
                }
            }
        }

        if (g.can_en_passant) {
            if (p.y==g.en_passant_y+1 && p.x==g.en_passant_x && g.past_y==g.en_passant_y) {
                return 1;
            }
        }
    }

    return 0;
}

uint8_t can_move_rook(uint8_t index, game_state g) {
    piece p = g.pieces[index];

    if (p.x==g.past_x) {
        // Check column for pieces in the way
        uint8_t i;
        for (i=0; i<32; i++)
            if (i!=index && g.pieces[i].taken==0 && g.pieces[i].x==p.x && ((g.pieces[i].y>=g.past_y && g.pieces[i].y<p.y) || (g.pieces[i].y<=g.past_y && g.pieces[i].y>p.y)))
                return 0; // Cannot move as pieces in the way

        // No pieces between select and move position
        for (i=0; i<32; i++)
            if (i!=index && g.pieces[i].taken==0 && g.pieces[i].x==p.x && g.pieces[i].y==p.y && g.pieces[i].team==g.turn)
                return 0; // Cannot take the piece we are on top of

        return 1;
    }

    if (p.y==g.past_y) {
        // Check row for pieces in the way
        uint8_t i;
        for (i=0; i<32; i++)
            if (i!=index && g.pieces[i].taken==0 && g.pieces[i].y==p.y && ((g.pieces[i].x>=g.past_x && g.pieces[i].x<p.x) || (g.pieces[i].x<=g.past_x && g.pieces[i].x>p.x)))
                return 0; // Cannot move as pieces are in the way

        for (i=0; i<32; i++)
            if (i!=index && g.pieces[i].taken==0 && g.pieces[i].x==p.x && g.pieces[i].y==p.y && g.pieces[i].team==g.turn)
                return 0; // Cannot take the piece we are on top of

        return 1;
    }

    return 0;
}

uint8_t can_move_knight(uint8_t index, game_state g) {
    piece p = g.pieces[index];

    uint8_t temp_type;

    if (((p.x==(g.past_x+1) || p.x==(g.past_x-1)) && (p.y==(g.past_y-2) || p.y==(g.past_y+2))) ||
        ((p.x==(g.past_x+2) || p.x==(g.past_x-2)) && (p.y==(g.past_y-1) || p.y==(g.past_y+1)))) { // Across 1, up / down 2 || Across 2, up / down 1
        if (g.turn==0) {
            temp_type = get_piece_type_at(p.x, p.y, g);
            if (temp_type>6) {
                return 1;
            } else if (temp_type==0) {
                return 1;
            } else {
                return 0;
            }
        } else {
            temp_type = get_piece_type_at(p.x, p.y, g);
            if (temp_type<7) {
                if (temp_type==0) {
                    return 1;
                } else {
                    return 1;
                }
            } else {
                return 0;
            }
        }
    }

    return 0;
}

uint8_t can_move_bishop(uint8_t index, game_state g) {
    piece p = g.pieces[index];

    uint8_t temp_type;

    uint8_t dif_x = calc_x_difference_from_past(p.x, g.past_x);
    uint8_t dif_y = calc_y_difference_from_past(p.y, g.past_y);

    if (dif_x==dif_y) { // If moved diagonally
        uint8_t i, p_dif_x, p_dif_y;
        for (i=0; i<32; i++) {
            if (i!=index) {
                p_dif_x = calc_x_difference_from_past(g.pieces[i].x, g.past_x);
                p_dif_y = calc_y_difference_from_past(g.pieces[i].y, g.past_y);

                if (p_dif_y==p_dif_x) { // Piece is on the diagonal and in the way (tested below)
                    if (((g.pieces[i].x>=g.past_x && g.pieces[i].x<p.x) || (g.pieces[i].x<=g.past_x && g.pieces[i].x>p.x)) && ((g.pieces[i].y>=g.past_y && g.pieces[i].y<p.y) || (g.pieces[i].y<=g.past_y && g.pieces[i].y>p.y)) && g.pieces[i].taken==0) {
                        return 0;
                    }
                }
            }
        }
        
        temp_type = get_piece_type_at(p.x, p.y, g);
        if (g.turn==0) {
            if (temp_type>6) {
                return 1;
            } else if (temp_type==0) {
                return 1;
            } else {
                return 0;
            }   
        } else {
            if (temp_type<7) {
                if (temp_type==0) {
                    return 1;
                } else {
                    return 1;
                }
            } else {
                return 0;
            }
        }
    }
    return 0;
}

uint8_t detect_castling(uint8_t index, game_state g) {
    // CASTLING DETECTION
    piece p = g.pieces[index];

    uint8_t turn_rook;
    if (g.turn) {
        turn_rook = 8;
    } else {
        turn_rook = 2;
    }

    uint8_t piece_type;
    uint8_t left  = 1;
    uint8_t right = 1;
    
    uint8_t j;
    if (p.first==1 && (g.past_x==(p.x+2) || g.past_x==(p.x-2)) && (p.y==0 || p.y==7)) {
        uint8_t left_piece_type  = get_piece_type_at(0, p.y, g);
        uint8_t left_piece_first = get_piece_first_state_at(0, p.y, g);
        uint8_t right_piece_type = get_piece_type_at(7, p.y, g);
        uint8_t right_piece_first = get_piece_first_state_at(7, p.y, g);

    
        if (left_piece_type!=turn_rook || left_piece_first==0) {
            left = 0;
        }

        if (right_piece_type!=turn_rook || right_piece_first==0) {
            right = 0;
        }
        

        if (left==0 && right==0) {
            return 0;
        }

        for (j=0; j<4; j++) {
            piece_type = get_piece_type_at(p.x-j, p.y, g);
            if (piece_type!=0 && piece_type!=turn_rook) {
                left = 0;
            }
        }

        for (j=0; j<3; j++) {
            piece_type = get_piece_type_at(p.x+j, p.y, g);
            if (piece_type!=0 && piece_type!=turn_rook) {
                right = 0;
            }
        }

        if (left || right) {
            return 1;
        }
    }

    return 0;
}

uint8_t can_move_king(uint8_t index, game_state g) {
    piece p = g.pieces[index];

    uint8_t temp_type;

    uint8_t dif_x = calc_x_difference_from_past(p.x, g.past_x);
    uint8_t dif_y = calc_y_difference_from_past(p.y, g.past_y);

    if (dif_x<=1 && dif_y<=1) {
        temp_type = get_piece_type_at(p.x, p.y, g);
        if (g.turn==0) {
            if (temp_type>6) {
                return 1;
            } else if (temp_type==0) {
                return 1;
            } else {
                return 0;
            }
        } else {
            if (temp_type<7) {
                if (temp_type==0) {
                    return 1;
                } else {
                    return 1;
                }  
            } else {
                return 0;
            }
        }
    } else return detect_castling(index, g);
}





uint8_t is_move_valid(piece p, game_state g) {
    if (p.x==g.past_x && p.y==g.past_y) return 1;

    uint8_t i;
    uint8_t index = 0;
    for (i=0; i<32; i++) {
        if (g.pieces[i].x==p.x && g.pieces[i].y==p.y && g.pieces[i].type==p.type && g.pieces[i].first==p.first) index = i;
    }

    // PAWN
    if (p.type==1 || p.type==7) return can_move_pawn(index, g);


    // ROOK
    if (p.type==2 || p.type==8) return can_move_rook(index, g);
    

    // KNIGHT
    if (p.type==3 || p.type==9) return can_move_knight(index, g);


    // BISHOP
    if (p.type==4 || p.type==10) return can_move_bishop(index, g);


    // QUEEN
    if (p.type==5 || p.type==11) return can_move_rook(index, g) || can_move_bishop(index, g);


    // KING
    if (p.type==6 || p.type==12) return can_move_king(index, g);

    return 0;
}

uint8_t check_in_check(game_state g) {
    uint8_t king_x;
    uint8_t king_y;

    uint8_t i;
    for (i=0; i<32; i++) {
        if (g.turn==0 && g.pieces[i].type==6) {
            king_x = g.pieces[i].x;
            king_y = g.pieces[i].y;
            break;
        } else if (g.turn && g.pieces[i].type==12) {
            king_x = g.pieces[i].x;
            king_y = g.pieces[i].y;
            break;
        } else {
            continue;
        }
    }

    //g.turn = (!g.turn);

    for (i=0; i<32; i++) {
        if (g.pieces[i].team!=g.turn && g.pieces[i].taken==0) {
            // IF CAN MOVE ONTO KING; CHECK AND BREAK
            g.past_x = g.pieces[i].x;
            g.past_y = g.pieces[i].y;

            g.pieces[i].x = king_x;
            g.pieces[i].y = king_y;
            if (is_move_valid(g.pieces[i], g)) {
                return 1;
            }

            // Restore to original position
            g.pieces[i].x = g.past_x;
            g.pieces[i].y = g.past_y;
        }
    }

    return 0;
}

uint8_t check_checkmate(game_state g) {
    if (check_in_check(g)) {
        if (are_there_possible_moves(g)) {
            return 1; // CHECK
        } else {
            return 2; // CHECKMATE
        }
    }

    return 0;
}






uint8_t will_take_piece(game_state g) {
    // HANDLE IF NEEDS TO TAKE A PIECE
    uint8_t i;
    for (i=0; i<32; i++) {
        // HANDLE IF NEEDS TO TAKE PIECE
        if (g.select.x==g.pieces[i].x && g.select.y==g.pieces[i].y && i!=g.selected_piece_index && g.pieces[i].taken==0) {
            return 1;
        }
    }

    // HANDLE EN PASSANT TAKING IF POSSIBLE
    for (i=0; i<32; i++) {
        if (g.can_en_passant && g.select.x==g.pieces[i].x && g.pieces[i].y==g.en_passant_y && g.pieces[i].x==g.en_passant_x && ((g.pieces[g.selected_piece_index].type==1 && g.select.y==g.en_passant_y-1) || (g.pieces[g.selected_piece_index].type==7 && g.select.y==g.en_passant_y+1)) && g.pieces[i].taken==0) {
            return 1;
        }
    }


    //for (i=0; i<32; i++) {
    //    // HANDLE IF DOES NOT NEED TO TAKE PIECE BECAUSE PIECE IS ALREADY TAKEN
    //    if (g.select.x==g.pieces[i].x && g.select.y==g.pieces[i].y && i!=g.selected_piece_index && g.pieces[i].taken==1) {
    //        return 1;
    //    }
    //}

    return 0;
}

game_state take_piece(game_state g) {
    // HANDLE IF NEEDS TO TAKE A PIECE
    uint8_t i;
    for (i=0; i<32; i++) {
        // HANDLE IF NEEDS TO TAKE PIECE
        if (g.select.x==g.pieces[i].x && g.select.y==g.pieces[i].y && i!=g.selected_piece_index && g.pieces[i].taken==0) {
            g.board_past_x = g.select.x;
            g.board_past_y = g.select.y;
            g.has_drawn = 0;
            g.pieces[i].taken = 1;
            g.select.active = 0;
            return g;
        }
    }

    // HANDLE EN PASSANT TAKING IF POSSIBLE
    for (i=0; i<32; i++) {
        if (g.can_en_passant && g.select.x==g.pieces[i].x && g.pieces[i].y==g.en_passant_y && g.pieces[i].x==g.en_passant_x && ((g.pieces[g.selected_piece_index].type==1 && g.select.y==g.en_passant_y-1) || (g.pieces[g.selected_piece_index].type==7 && g.select.y==g.en_passant_y+1)) && g.pieces[i].taken==0) {
            g.board_past_x = g.select.x;
            g.board_past_y = g.select.y;
            g.en_passant_occured = 1;
            g.has_drawn = 0;
            g.pieces[i].taken = 1;
            g.select.active = 0;
            return g;
        }
    }


    //for (i=0; i<32; i++) {
    //    // HANDLE IF DOES NOT NEED TO TAKE PIECE BECAUSE PIECE IS ALREADY TAKEN
    //    if (g.select.x==g.pieces[i].x && g.select.y==g.pieces[i].y && i!=g.selected_piece_index && g.pieces[i].taken==1) {
    //        g.board_past_x = g.select.x;
    //        g.board_past_y = g.select.y;
    //        g.has_drawn = 0;
    //        g.select.active = 0;
    //        return g;
    //    }
    //}

    return g;
}

rectangle get_piece_rectangle_from_coords(uint8_t x, uint8_t y, game_state g) {
    rectangle r = {40+(x)*TILESIZE+g.select.thickness,40+((x)*TILESIZE)+(TILESIZE-1)-g.select.thickness,(int)(y)*TILESIZE+g.select.thickness,((int)(y)*TILESIZE)+(TILESIZE-1)-g.select.thickness};

    return r;
}

game_state move_and_possibly_take_piece(game_state g) {
    if (will_take_piece(g)==0) {
        // HANDLE CASTLING MOVEMENT IF OCCURING
        uint8_t cst = detect_castling(g.selected_piece_index, g);
        if ((g.pieces[g.selected_piece_index].type==6 || g.pieces[g.selected_piece_index].type==12) &&
             cst) {
            // CASTLING DETECTION
            g.castling_occured = 1;

            uint8_t index;
            if (g.turn==0) {
                if (g.select.x==6) {
                    // RIGHT
                    index = get_piece_index_by_type(2, 7, 7, g);
                    g.pieces[index].r = get_piece_rectangle_from_coords(g.select.x-1, g.select.y, g);
                    g.pieces[index].x = 5;
                    g.pieces[index].first = 0;

                    g.board_past_x = g.select.x-1;
                    g.board_past_y = g.select.y;
                    g.has_drawn = 0;
                    g.select.active = 0;
                } else if (g.select.x==2) {
                    // LEFT
                    index = get_piece_index_by_type(2, 0, 7, g);
                    g.pieces[index].r = get_piece_rectangle_from_coords(g.select.x+1, g.select.y, g);
                    g.pieces[index].x = 3;
                    g.pieces[index].first = 0;

                    g.board_past_x = g.select.x+1;
                    g.board_past_y = g.select.y;
                    g.has_drawn = 0;
                    g.select.active = 0;                      
                }
            } else {
                if (g.select.x==6) {
                    // RIGHT
                    index = get_piece_index_by_type(8, 7, 0, g);
                    g.pieces[index].r = get_piece_rectangle_from_coords(g.select.x-1, g.select.y, g);
                    g.pieces[index].x = 5;
                    g.pieces[index].first = 0;

                    g.board_past_x = g.select.x-1;
                    g.board_past_y = g.select.y;
                    g.has_drawn = 0;
                    g.select.active = 0;
                } else if (g.select.x==2) {
                    // LEFT
                    index = get_piece_index_by_type(8, 0, 0, g);
                    g.pieces[index].r = get_piece_rectangle_from_coords(g.select.x+1, g.select.y, g);
                    g.pieces[index].x = 3;
                    g.pieces[index].first = 0;

                    g.board_past_x = g.select.x+1;
                    g.board_past_y = g.select.y;
                    g.has_drawn = 0;
                    g.select.active = 0;                       
                }
            }
        } else {
            // CATCH ALL FOR JUST GENERIC MOVEMENT
            g.board_past_x = g.select.x;
            g.board_past_y = g.select.y;
            g.has_drawn = 0;
            g.select.active = 0;
        }
    } else {
        g = take_piece(g);
    } 

    // SET FLAG SHOWING PIECE MOVEMENT HAS OCCURED
    g.pieces[g.selected_piece_index].first = 0;

    // RESET EN PASSANT FLAG TO AVOID CONFLICTS
    g.can_en_passant = 0;

    // Set flags ready that en_passant can occur in the next move
    if (g.pieces[g.selected_piece_index].type==1) {
        if (g.pieces[g.selected_piece_index].y==(g.past_y-2)) {
            g.can_en_passant = 1;
            g.en_passant_x = g.pieces[g.selected_piece_index].x;
            g.en_passant_y = g.pieces[g.selected_piece_index].y;
        }
    }

    if (g.pieces[g.selected_piece_index].type==7) {
        if (g.pieces[g.selected_piece_index].y==(g.past_y+2)) {
            g.can_en_passant = 1;
            g.en_passant_x = g.pieces[g.selected_piece_index].x;
            g.en_passant_y = g.pieces[g.selected_piece_index].y;
        }
    }

    return g;
}






uint8_t is_turn_valid(piece p, game_state g) {
    if (is_move_valid(p, g)) {
        g = move_and_possibly_take_piece(g);
        if (check_in_check(g)) return 0;
        else return 1;
    }

    return 0;
}






uint8_t is_possible_move_for_piece(piece p, game_state g) {
    uint8_t index = get_piece_index_in_game_state(p, g);

    g.past_x = p.x;
    g.past_y = p.y;

    uint8_t i, j;
    for (i=0; i<8; i++) {
        for (j=0; j<8; j++) {
            p.x = j;
            p.y = i;

            g.pieces[index] = p;

            if (is_turn_valid(p, g)) {
                return 1;
            }
        }
    }

    return 0;
}

uint8_t are_there_possible_moves(game_state g) {
    uint8_t i;
    for (i=0; i<32; i++) {
        if (g.pieces[i].team==g.turn && g.pieces[i].taken==0) {
            if (is_possible_move_for_piece(g.pieces[i], g)) {
                return 1;
            }
        }
    }

    return 0;
}

move_set get_possible_moves_for_piece(piece p, game_state g) {
    move_set m_s;

    uint8_t index = get_piece_index_in_game_state(p, g);

    g.past_x = p.x;
    g.past_y = p.y;

    uint8_t counter = 0;
    uint8_t i, j;
    for (i=0; i<8; i++) {
        for (j=0; j<8; j++) {

            p.x = j;
            p.y = i;

            g.pieces[index] = p;

            if (is_turn_valid(p, g)) {
                m_s.possible_moves_x[counter] = j;
                m_s.possible_moves_y[counter] = i;
                counter++;
            }
        }
    }

    m_s.num_possible_moves = counter;

    return m_s;
}






uint8_t is_pawn_at_other_side() {
    uint8_t i;
    for (i=0; i<32; i++) {
        if (current_state.pieces[i].team==0 && current_state.pieces[i].type==1 && current_state.pieces[i].y==0) return 1;

        if (current_state.pieces[i].team==1 && current_state.pieces[i].type==7 && current_state.pieces[i].y==7) return 1;
    }

    return 0;
}

uint8_t get_index_of_pawn_at_other_side() {
    uint8_t i;
    for (i=0; i<32; i++) {
        if (current_state.pieces[i].team==0 && current_state.pieces[i].type==1 && current_state.pieces[i].y==0) return i;

        if (current_state.pieces[i].team==1 && current_state.pieces[i].type==7 && current_state.pieces[i].y==7) return i;
    }

    return 0;
}

void check_switches() {
    cli();

    if (is_pawn_at_other_side() && current_state.select.active==0) { // STALL ALL OTHER INPUT CHANGES UNTIL PAWN NO LONGER AT OTHER SIDE
        uint8_t index = get_index_of_pawn_at_other_side();
        uint8_t team = current_state.pieces[index].team;
        current_state.select.x = current_state.pieces[index].x;
        current_state.select.y = current_state.pieces[index].y;
        current_state.select.col = MAGENTA;
        current_state.select.active = 0;

        if (get_switch_press(_BV(SWE))) {
            current_state.pieces[index].s = &queen;
            current_state.board_past_x = current_state.select.x;
            current_state.board_past_y = current_state.select.y;
            current_state.has_drawn = 0;
        }

        if (get_switch_press(_BV(SWW))) {
            current_state.pieces[index].s = &knight;
            current_state.board_past_x = current_state.select.x;
            current_state.board_past_y = current_state.select.y;
            current_state.has_drawn = 0;
        }

        if (get_switch_press(_BV(SWC))) {
            if (current_state.pieces[index].s==&knight) {
                if (team) {
                    current_state.pieces[index].type = 9;
                } else {
                    current_state.pieces[index].type = 3;
                }

                current_state.board_past_x = current_state.select.x;
                current_state.board_past_y = current_state.select.y;
                current_state.has_drawn = 0;
                current_state.select.active = 0;

                update_selected();
            } else if (current_state.pieces[index].s==&queen) {
                if (team) {
                    current_state.pieces[index].type = 11;
                } else {
                    current_state.pieces[index].type = 5;
                }

                current_state.board_past_x = current_state.select.x;
                current_state.board_past_y = current_state.select.y;
                current_state.has_drawn = 0;
                current_state.select.active = 0;

                update_selected();
            }
        }
    } else {
        if (get_switch_press(_BV(SWE))) {
            if (current_state.select.x<7) {
                current_state.select.r.left += TILESIZE;
                current_state.select.r.right += TILESIZE;
                current_state.select.x++;

                if (current_state.select.x>0) {
                    current_state.board_past_x = current_state.select.x-1;
                    current_state.board_past_y = current_state.select.y;
                } else {
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y;
                }
                current_state.has_drawn = 0;
            }
        }

        if (get_switch_press(_BV(SWW))) {
            if (current_state.select.x>0) {
                current_state.select.r.left -= TILESIZE;
                current_state.select.r.right -= TILESIZE;
                current_state.select.x--;

                if (current_state.select.x<7) {
                    current_state.board_past_x = current_state.select.x+1;
                    current_state.board_past_y = current_state.select.y;
                } else {
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y;
                }
                current_state.has_drawn = 0;
            }
        }

        if (get_switch_press(_BV(SWS))) {
            if (current_state.select.y<7) {
                current_state.select.r.top += TILESIZE;
                current_state.select.r.bottom += TILESIZE;
                current_state.select.y++;
                    
                if (current_state.select.y>0) {
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y-1;
                } else {
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y;
                }
                current_state.has_drawn = 0;
            }
        }

        if (get_switch_press(_BV(SWN))) {
            if (current_state.select.y>0) {
                current_state.select.r.top -= TILESIZE;
                current_state.select.r.bottom -= TILESIZE;
                current_state.select.y--;

                if (current_state.select.y<7) {
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y+1;
                } else {
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y;
                }
                current_state.has_drawn = 0;
            }
        }

        update_selected();

        if (get_switch_press(_BV(SWC))) {
            if (current_state.select.active==1) {
                if (current_state.pieces[current_state.selected_piece_index].x==current_state.past_x && current_state.pieces[current_state.selected_piece_index].y==current_state.past_y) {
                    update_selected();
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y;
                    current_state.has_drawn = 0;
                    current_state.select.active = 0;
                } else {
                    if (is_turn_valid(current_state.pieces[current_state.selected_piece_index], current_state)) {
                        current_state = move_and_possibly_take_piece(current_state);
                        change_turn();
                    }
                }
            } else {
                uint8_t i;
                for (i=0; i<32; i++) {
                    if (current_state.pieces[i].x==current_state.select.x && current_state.pieces[i].y==current_state.select.y && current_state.pieces[i].team==current_state.turn && current_state.pieces[i].taken==0) {
                        current_state.selected_piece_index = i;
                        current_state.select.active = 1;
                        current_state.past_x = current_state.select.x;
                        current_state.past_y = current_state.select.y;
                        break;
                    }
                }

                if (current_state.select.active) {
                    current_state.board_past_x = current_state.select.x;
                    current_state.board_past_y = current_state.select.y;
                    current_state.has_drawn = 0;
                    current_move_set = get_possible_moves_for_piece(current_state.pieces[current_state.selected_piece_index], current_state);
                }
            }
        }

        update_selected();
    }

    sei();
}





void change_turn() {
    if (current_state.turn) current_state.turn = 0;
    else current_state.turn = 1;
}





rectangle get_rectangle_for_selector() {
    rectangle r;
    r.left = current_state.select.r.left+current_state.select.thickness;
    r.right = current_state.select.r.right-current_state.select.thickness;
    r.top = current_state.select.r.top+current_state.select.thickness;
    r.bottom = current_state.select.r.bottom-current_state.select.thickness;

    return r;
}

void update_selected() {
    if (current_state.select.active) {
        if ((is_turn_valid(current_state.pieces[current_state.selected_piece_index], current_state)) || 
            (current_state.pieces[current_state.selected_piece_index].x==current_state.past_x && current_state.pieces[current_state.selected_piece_index].y==current_state.past_y)) {
            current_state.select.col = 0x07FF; // CYAN
        } else {
            current_state.select.col = 0xF800; // RED
        }

        current_state.pieces[current_state.selected_piece_index].r = get_rectangle_for_selector(current_state.pieces[current_state.selected_piece_index]);
        current_state.pieces[current_state.selected_piece_index].x = current_state.select.x;
        current_state.pieces[current_state.selected_piece_index].y = current_state.select.y;
    } else {
        uint8_t i;
        for (i=0; i<32; i++) {
            if (current_state.pieces[i].x==current_state.select.x && current_state.pieces[i].y==current_state.select.y && current_state.pieces[i].team==current_state.turn && current_state.pieces[i].taken==0) {
                current_state.select.col = 0x07E0; // GREEN
                return;
            }
        }

        current_state.select.col = 0xFFE0; // YELLOW
    }
}






void create_board() {
    uint8_t i;
    /* Create Tiles */
    for (i=0; i<64; i++) {
        rectangle r = {40+(i%8)*TILESIZE,40+((i%8)*TILESIZE)+(TILESIZE-1),(int)(i/8)*TILESIZE,((int)(i/8)*TILESIZE)+(TILESIZE-1)};
        tile t;
        t.r = r;
        if (i/8%2==0) {
            if (i%2==0) {
                t.col = LIGHT_BROWN; // Light brown instead of white
                board[i] = t;
            } else {
                t.col = DARK_BROWN; // Dark brown instead of black
                board[i] = t;
            }
        } else {
            if (i%2==0) {
                t.col = DARK_BROWN;
                board[i] = t;
            } else {
                t.col = LIGHT_BROWN;
                board[i] = t;
            }
        }
    }
}

void create_pieces() {
    uint8_t i;

    // Type (WHITE): 1 (Pawn), 2 (Rook), 3 (Knight), 4 (Bishop), 5 (Queen), 6 (King)
    // Type (BLACK): 7 (Pawn), 8 (Rook), 9 (Knight),10 (Bishop),11 (Queen),12 (King)

    // TODO CREATE CORRECT PIECES AND PLACE THEM CORRECTLY
    // Create pieces
    for (i=0; i<32; i++) {
        piece p;
        
        if (i/16==0) {
            rectangle r = {40+(i%8)*TILESIZE+current_state.select.thickness,40+((i%8)*TILESIZE)+(TILESIZE-1)-current_state.select.thickness,(int)(i/8)*TILESIZE+current_state.select.thickness,((int)(i/8)*TILESIZE)+(TILESIZE-1)-current_state.select.thickness};

            p.r = r;
            p.x = (i%8);
            p.y = (i/8);
            p.taken = 0;
            p.first = 1; // TRUE
            p.team = 1;
            if (i==0 || i==7) {
                p.type = 8;
                p.s = &rook;
            } else if (i==1 || i==6) { 
                p.type = 9;
                p.s = &knight;
            } else if (i==2 || i==5) { 
                p.type = 10;
                p.s = &bishop;
            } else if (i==3) {
                p.type = 11;
                p.s = &queen;
            } else if (i==4) {
                p.type = 12;
                p.s = &king;
            } else {
                p.type = 7;
                p.s = &pawn;
            }
        } else {
            rectangle r = {40+(i%8)*TILESIZE+current_state.select.thickness,40+((i%8)*TILESIZE)+(TILESIZE-1)-current_state.select.thickness,((int)(9-(i/8))*TILESIZE+current_state.select.thickness),(((int)(9-(i/8))*TILESIZE)+(TILESIZE-1)-current_state.select.thickness)};
            
            p.r = r;
            p.x = (i%8);
            p.y = 9-(i/8);
            p.taken = 0;
            p.first = 1; // TRUE
            p.team = 0;
            if (i==16 || i==23) {
                p.type = 2;
                p.s = &rook;
            } else if (i==17 || i==22) { 
                p.type = 3;
                p.s = &knight;
            } else if (i==18 || i==21) { 
                p.type = 4;
                p.s = &bishop;
            } else if (i==19) {
                p.type = 5;
                p.s = &queen;
            } else if (i==20) {
                p.type = 6;
                p.s = &king;
            } else {
                p.type = 1;
                p.s = &pawn;
            }
        }
        
        current_state.pieces[i] = p;
    }
}

void create_selector() {
    /* Create Selector */
    current_state.select.r.left = 40;
    current_state.select.r.right = 69;
    current_state.select.r.top = 0;
    current_state.select.r.bottom = 29;
    current_state.select.thickness = 3;
    current_state.select.col = 0x07E0;
    current_state.select.x = 0;
    current_state.select.y = 0;
    current_state.select.active = 0;
}

void first_draw() {
    uint8_t i;
    for (i=0; i<64; i++) draw_tile(board[i]);
    for (i=0; i<32; i++)
        draw_piece(current_state.pieces[i]);
    
    draw_select(current_state.select);
}

void init_game() {
    create_board();

    create_selector();  

    // SELECTOR THICKNESS NEEDED TO MAKE PIECES
    create_pieces();

    current_state.turn = 0; 

    current_state.selected_piece_index = 0; 

    current_state.can_en_passant = 0;

    current_state.select_active_last_draw = 0;
    current_state.board_past_x = 0;
    current_state.board_past_y = 0;
    current_state.en_passant_occured = 0;
    current_state.castling_occured = 0;

    first_draw();
}






void draw_possible_moves() {
    uint8_t i;

    for (i=0; i<current_move_set.num_possible_moves; i++) {
        rectangle r = {40+(30*current_move_set.possible_moves_x[i])+11, 40+(30*current_move_set.possible_moves_x[i])+19, (30*current_move_set.possible_moves_y[i])+11, (30*current_move_set.possible_moves_y[i])+19};
        fill_rectangle(r, CYAN);
    }
}

void draw_select(selector s) {
    rectangle r1 = {s.r.left, s.r.right, s.r.top, s.r.top+s.thickness};
    rectangle r2 = {s.r.left, s.r.right, s.r.bottom-s.thickness, s.r.bottom};
    rectangle r3 = {s.r.left, s.r.left+s.thickness, s.r.top, s.r.bottom};
    rectangle r4 = {s.r.right-s.thickness, s.r.right, s.r.top, s.r.bottom};

    fill_rectangle(r1, s.col);
    fill_rectangle(r2, s.col);
    fill_rectangle(r3, s.col);
    fill_rectangle(r4, s.col);
}

void draw_piece(piece p) {
    write_cmd(COLUMN_ADDRESS_SET);
    write_data16(p.r.left);         // Left coord
    write_data16(p.r.right);        // Right coord
    write_cmd(PAGE_ADDRESS_SET);
    write_data16(p.r.top);          // Top coord
    write_data16(p.r.bottom);       // Bottom coord
    write_cmd(MEMORY_WRITE);

    uint8_t i, j, k;
    
    if (p.y%2==0) {
        if (p.x%2==0) {
            // DARK_BROWN
            for (i=0; i<24; i++) {
                for (j=0; j<3;j++) {
                    for (k=0; k<8; k++) {
                        uint8_t val = (uint8_t)((p.s->data[i][j])<<k) & 0x80;
                        if (val) {
                            if (p.team) {
                                write_data16(0x0000);
                            } else {
                                write_data16(0xFFFF);
                            }
                        } else {
                            write_data16(LIGHT_BROWN);
                        }
                    }
                }
            }
        } else {
            // LIGHT_BROWN
            for (i=0; i<24; i++) {
                for (j=0; j<3;j++) {
                    for (k=0; k<8; k++) {
                        uint8_t val = (uint8_t)((p.s->data[i][j])<<k) & 0x80;
                        if (val) {
                            if (p.team) {
                                write_data16(0x0000);
                            } else {
                                write_data16(0xFFFF);
                            }
                        } else {
                            write_data16(DARK_BROWN);
                        }
                    }
                }
            }
        }
    } else {
        if (p.x%2==0) {
            // LIGHT_BROWN
            for (i=0; i<24; i++) {
                for (j=0; j<3;j++) {
                    for (k=0; k<8; k++) {
                        uint8_t val = (uint8_t)((p.s->data[i][j])<<k) & 0x80;
                        if (val) {
                            if (p.team) {
                                write_data16(0x0000);
                            } else {
                                write_data16(0xFFFF);
                            }
                        } else {
                            write_data16(DARK_BROWN);
                        }
                    }
                }
            }
        } else {
            // DARK_BROWN
            for (i=0; i<24; i++) {
                for (j=0; j<3;j++) {
                    for (k=0; k<8; k++) {
                        uint8_t val = (uint8_t)((p.s->data[i][j])<<k) & 0x80;
                        if (val) {
                            if (p.team) {
                                write_data16(0x0000);
                            } else {
                                write_data16(0xFFFF);
                            }
                        } else {
                            write_data16(LIGHT_BROWN);
                        }
                    }
                }
            }
        }
    }
}

void draw_tile(tile t) {
    fill_rectangle(t.r, t.col);
}






void refresh_tile(uint8_t x, uint8_t y, game_state g) {
    rectangle r = {40+(x*TILESIZE),40+(x*TILESIZE)+(TILESIZE-1),y*TILESIZE,(y*TILESIZE)+(TILESIZE-1)};
    tile t;
    t.r = r;
    if (y%2==0) {
        if (x%2==0) {
            t.col = LIGHT_BROWN; // Light brown instead of white
        } else {
            t.col = DARK_BROWN; // Dark brown instead of black
        }
    } else {
        if (x%2==0) {
            t.col = DARK_BROWN;
        } else {
            t.col = LIGHT_BROWN;
        }
    }

    draw_tile(t);

    uint8_t i;
    for (i=0; i<32; i++) {
        if (g.select.active) {
            if (i!=g.selected_piece_index && g.pieces[i].x==x && g.pieces[i].y==y && g.pieces[i].taken==0) draw_piece(g.pieces[i]);
        } else if (g.pieces[i].x==x && g.pieces[i].y==y && g.pieces[i].taken==0) draw_piece(g.pieces[i]);
    }
}

void draw_over_potential_moves(move_set m_s, game_state g) {
    uint8_t i;
    for (i=0; i<m_s.num_possible_moves; i++) {
        refresh_tile(m_s.possible_moves_x[i], m_s.possible_moves_y[i], g);
    }
}

void refresh_en_passant_tile(game_state g) {
    refresh_tile(g.en_passant_x, g.en_passant_y, g);
}

void refresh_castling_tile(game_state g) {
    refresh_tile(0, g.board_past_y, g);
    refresh_tile(7, g.board_past_y, g);
}






int main() {
    /* Clear DIV8 to get 8MHz clock */
	CLKPR = (1 << CLKPCE);
	CLKPR = 0;

    init_lcd();
    init_buttons();
    set_frame_rate_hz(50);

    /* Enable tearing interrupt to get flicker free display */
    EIMSK |= _BV(INT6);

    /* Enable rotary interrupt to respond to input */
	EIMSK |= _BV(INT4) | _BV(INT5);

    /* Enable timer (Timer 1 CTC Mode 4) */
	TCCR1A = 0;
	TCCR1B = _BV(WGM12);
	TCCR1B |= _BV(CS10);
	TIMSK1 |= _BV(OCIE1A);
    OCR1A = 65535;

    /* Enable performance counter (Timer 3 CTC Mode 4) */
	TCCR3A = 0;
	TCCR3B = _BV(WGM32);
	TCCR3B |= _BV(CS32);
	TIMSK3 |= _BV(OCIE3A);
	OCR3A = 31250;
    

    current_state.has_drawn = 0;
    
    init_game();

    // Enable interrupts */
    sei();

    uint8_t checkmate_state;
    do {
        // Spin wheels for time being
        check_switches();

        if (current_state.has_drawn==0) {
            checkmate_state = check_checkmate(current_state);
            //checkmate_state = check_in_check(current_state);
            if (checkmate_state!=2) {
                if (checkmate_state) {
                    rectangle r = {0,20,0,20};
                    fill_rectangle(r, RED);
                } else {
                    rectangle r = {0,20,0,20};
                    fill_rectangle(r, BLACK);
                }




                // DRAW ONLY NEEDED REGION
                if (current_state.en_passant_occured) {
                    refresh_en_passant_tile(current_state);
                    current_state.en_passant_occured = 0;
                }
                if (current_state.castling_occured) {
                    refresh_castling_tile(current_state);
                    current_state.castling_occured = 0;
                }


                if (current_state.board_past_x!=current_state.select.x || current_state.board_past_y!=current_state.select.y) {
                    refresh_tile(current_state.board_past_x, current_state.board_past_y, current_state);
                }
                refresh_tile(current_state.select.x, current_state.select.y, current_state);
    
                if (current_state.select.active) {
                    // DRAW SELECTED PIECE ON TOP OF ALL OTHERS
                    draw_piece(current_state.pieces[current_state.selected_piece_index]);
                    draw_possible_moves();
                    current_state.select_active_last_draw = 1;
                } else {
                    if (current_state.select_active_last_draw==1) {
                        current_state.select_active_last_draw = 0;
                        draw_over_potential_moves(current_move_set, current_state);
                        // DRAW SELECTED PIECE ON TOP OF ALL OTHERS
                        draw_piece(current_state.pieces[current_state.selected_piece_index]);
                    }
                }
    
                draw_select(current_state.select);

                current_state.has_drawn=1;
            } else {
                rectangle r = {0,100,0,100};
                fill_rectangle(r, MAGENTA);
                do {} while (1);
            }
        }
    } while (1);

    return 0;
}
