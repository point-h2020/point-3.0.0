/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.client;

import java.util.concurrent.SynchronousQueue;

import eu.point.tmsdn.impl.TmSdnMessages;
import io.netty.channel.ChannelHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelInboundHandlerAdapter;

/**
 * The connection handler class.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
@ChannelHandler.Sharable
public class TmSdnClientHandler extends ChannelInboundHandlerAdapter {

    public SynchronousQueue<TmSdnMessages.TmSdnMessage.ResourceOfferMessage> resultQueue;
    private TmSdnMessages.TmSdnMessage message;

    /**
     * The constructor of the handler.
     */
    public TmSdnClientHandler(TmSdnMessages.TmSdnMessage message) {
        this.message = message;
        this.resultQueue = new SynchronousQueue<TmSdnMessages.TmSdnMessage.ResourceOfferMessage>();
    }


    @Override
    public void channelActive(ChannelHandlerContext ctx) {
        ctx.writeAndFlush(message);
    }


    @Override
    public void channelRead(ChannelHandlerContext ctx, Object msg) {
        if (msg instanceof TmSdnMessages.TmSdnMessage) {
            try {
                resultQueue.put(((TmSdnMessages.TmSdnMessage) msg).getResourceOfferMessage());
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
