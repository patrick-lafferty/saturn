#!/bin/sh

lines="$(tput lines)"
columns="$(tput cols)"

tmux -2 new-session -d -s saturn -x "$columns" -y "$lines"
tmux new-window -a -n saturn

# super advanced hacker layout
tmux split-window -h -p 30 

# right side
tmux split-window -v -p 90 #memory
tmux split-window -v -p 60 #stack
tmux split-window -v -p 80 #threads

# left side
tmux select-pane -t 0
tmux split-window -v -p 66 #source
tmux split-window -v -p 40 #variables 
tmux select-pane -t 1 
tmux split-window -h -p 35 #assembly and registers

tmux select-pane -t 7
tmux attach -t saturn