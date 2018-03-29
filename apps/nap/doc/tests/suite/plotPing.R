options(echo=TRUE) # if you want see commands in output file
args <- commandArgs(trailingOnly = TRUE)
print(args)
# trailingOnly=TRUE means that only your arguments are returned, check:
# print(commandArgs(trailingOnly=FALSE))

print (args[1])

datApu <- read.csv(args[1], header = FALSE)
png(paste(args[1], ".png", sep=""),800,600)
matplot(datApu, type="l", xlab="Time [s]", ylab="RTT [ms]")
grid()
graphics.off()

rm(args)

