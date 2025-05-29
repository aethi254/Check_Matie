import json
import copy  # use it for deepcopy if needed
import math  # for math.inf
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

# Global variables in which you need to store player strategies (this is data structure that'll be used for evaluation)
# Mapping from histories (str) to probability distribution over actions
strategy_dict_x = {}
strategy_dict_o = {}


class History:
    def __init__(self, history=None):
        """
        # self.history : Eg: [0, 4, 2, 5]
            keeps track of sequence of actions played since the beginning of the game.
            Each action is an integer between 0-8 representing the square in which the move will be played as shown
            below.
              ___ ___ ____
             |_0_|_1_|_2_|
             |_3_|_4_|_5_|
             |_6_|_7_|_8_|

        # self.board
            empty squares are represented using '0' and occupied squares are either 'x' or 'o'.
            Eg: ['x', '0', 'x', '0', 'o', 'o', '0', '0', '0']
            for board
              ___ ___ ____
             |_x_|___|_x_|
             |___|_o_|_o_|
             |___|___|___|

        # self.player: 'x' or 'o'
            Player whose turn it is at the current history/board

        :param history: list keeps track of sequence of actions played since the beginning of the game.
        """
        if history is not None:
            self.history = history
            self.board = self.get_board()
        else:
            self.history = []
            self.board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        self.player = self.current_player()

    def current_player(self):
        """ Player function
        Get player whose turn it is at the current history/board
        :return: 'x' or 'o' or None
        """
        total_num_moves = len(self.history)
        if total_num_moves < 9:
            if total_num_moves % 2 == 0:
                return 'x'
            else:
                return 'o'
        else:
            return None

    def get_board(self):
        """ Play out the current self.history and get the board corresponding to the history in self.board.

        :return: list Eg: ['x', '0', 'x', '0', 'o', 'o', '0', '0', '0']
        """
        board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        for i in range(len(self.history)):
            if i % 2 == 0:
                board[self.history[i]] = 'x'
            else:
                board[self.history[i]] = 'o'
        return board

    def is_win(self):
        # check if the board position is a win for either players
        # Feel free to implement this in anyway if needed
        b=self.board
        lines=[(0,1,2),(3,4,5),(6,7,8),(0,3,6),(1,4,7),(2,5,8),(0,4,8),(2,4,6)]
        for i,j,k in lines:
            if b[i] == b[j] == b[k] and b[i] in ('x','o'):
                return True
        return False

    def is_draw(self):
        # check if the board position is a draw
        # Feel free to implement this in anyway if needed
        count=0
        a=False
        for i in self.board:
            if i=='0':
                count+=1
        if count == 0:
            a=True
        return a and not self.is_win()

    def get_valid_actions(self):
        # get the empty squares from the board
        # Feel free to implement this in anyway if needed
        return [i for i,cell in enumerate(self.board) if cell == '0']

    def is_terminal_history(self):
        # check if the history is a terminal history
        # Feel free to implement this in anyway if needed
        return self.is_win() or self.is_draw()

    def get_utility_given_terminal_history(self):
        # Feel free to implement this in anyway if needed
        if self.is_draw():
            return 0.0
        elif self.is_win():
            # The winner is the player who made the last move
            return 1.0 if (len(self.history) % 2 == 1) else -1.0

    def update_history(self, action):
        # In case you need to create a deepcopy and update the history obj to get the next history object.
        # Feel free to implement this in anyway if needed
        return History(self.history + [action])


def backward_induction(h):
    """
    :param h: History class object
    :return: best achievable utility (float) for the current history_obj
    """
    global strategy_dict_x, strategy_dict_o
    
    if h.is_terminal_history():
        return h.get_utility_given_terminal_history()
    
    key = ''.join(str(i) for i in h.history)
    
    if h.player == 'x':
        best_utility = -2
        best_action = None
        action_dict = {"0": 0.0, "1": 0.0, "2": 0.0, "3": 0.0, "4": 0.0, "5": 0.0, "6": 0.0, "7": 0.0, "8": 0.0}
        
        for action in h.get_valid_actions():
            # Create new history for next state
            new_history = History(h.history + [action])
            utility = backward_induction(new_history)
            
            if utility > best_utility:
                best_utility = utility
                best_action = action
        
        action_dict[str(best_action)] = 1.0
        strategy_dict_x[key] = action_dict
        return best_utility
        
    elif h.player == 'o':
        best_utility = 2
        best_action = None
        action_dict = {"0": 0.0, "1": 0.0, "2": 0.0, "3": 0.0, "4": 0.0, "5": 0.0, "6": 0.0, "7": 0.0, "8": 0.0}
        
        for action in h.get_valid_actions():
            # Create new history for next state
            new_history = History(h.history + [action])
            utility = backward_induction(new_history)
            
            if utility < best_utility:
                best_utility = utility
                best_action = action
        
        action_dict[str(best_action)] = 1.0
        strategy_dict_o[key] = action_dict
        return best_utility


def solve_tictactoe():
    backward_induction(History())
    with open('./policy_x.json', 'w') as f:
        json.dump(strategy_dict_x, f)
    with open('./policy_o.json', 'w') as f:
        json.dump(strategy_dict_o, f)
    return strategy_dict_x, strategy_dict_o


if __name__ == "__main__":
    logging.info("Start")
    solve_tictactoe()
    logging.info("End")
