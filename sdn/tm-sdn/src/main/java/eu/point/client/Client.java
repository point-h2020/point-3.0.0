/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.client;

import eu.point.tmsdn.impl.TmSdnMessages;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelOption;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.socket.nio.NioSocketChannel;
import io.netty.channel.socket.SocketChannel;
import io.netty.handler.codec.protobuf.ProtobufDecoder;
import io.netty.handler.codec.protobuf.ProtobufEncoder;
import io.netty.handler.codec.protobuf.ProtobufVarint32FrameDecoder;
import io.netty.handler.codec.protobuf.ProtobufVarint32LengthFieldPrepender;
import io.netty.handler.timeout.ReadTimeoutHandler;

/**
 * The TM-SDN client class.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class Client {

    // default variables
    public static int tcpPort = 12345;
    public static String serverIP = "127.0.0.1";
    private static TmSdnClientHandler handler;

    /**
     * The constructor of the client.
     */
    public Client() {

    }


    /**
     * The method which creates the bootstrap to connect to the server.
     * It initializes the pipeline with all the required parameters and functions
     * for encoding, decoding, etc.
     * It then sends the message to the server.
     * If the message is a Resource Request, then it also returns the received Resource Offer.
     *
     * @param message The message to be sent.
     * @return The received Resource Offer in case the message was a Resource Request.
     */
    public static TmSdnMessages.TmSdnMessage.ResourceOfferMessage sendMessage(TmSdnMessages.TmSdnMessage message) {
        EventLoopGroup group = new NioEventLoopGroup();
        TmSdnMessages.TmSdnMessage.ResourceOfferMessage resourceOffer = null;
        handler = new TmSdnClientHandler(message);
        try {
            Bootstrap b = new Bootstrap();
            b.group(group)
                    .channel(NioSocketChannel.class);
            b.option(ChannelOption.SO_KEEPALIVE, false);
            //initialize the pipeline
            b.handler(new ChannelInitializer<SocketChannel>() {
                @Override
                protected void initChannel(SocketChannel ch) throws Exception {
                    ChannelPipeline pipeline = ch.pipeline();

                    pipeline.addLast(new ProtobufVarint32FrameDecoder());
                    pipeline.addLast(new ProtobufDecoder(
                            TmSdnMessages.TmSdnMessage.getDefaultInstance()));
                    pipeline.addLast(new ProtobufVarint32LengthFieldPrepender());
                    pipeline.addLast(new ProtobufEncoder());
                    pipeline.addLast("readTimeoutHandler", new ReadTimeoutHandler(30));

                    pipeline.addLast(handler);
                }
            });

            // Make a new connection.
            ChannelFuture f = b.remoteAddress(serverIP, tcpPort).connect();
            try {
                f.sync();
            } catch (Exception e) {
                e.printStackTrace();
            }
            //if message is resource request, then get the received resource offer and return it.
            if (f.isSuccess() && message.getType() == TmSdnMessages.TmSdnMessage.TmSdnMessageType.RR) {
                TmSdnMessages.TmSdnMessage.ResourceOfferMessage resp = handler.resultQueue.take();
                if (resp != null) {
                    resourceOffer = resp;
                }
            }
            //close the channel.
            f.channel().closeFuture().sync();

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            group.shutdownGracefully();
        }
        return resourceOffer;
    }

}
